import importlib.util
import pathlib
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("analyze_microstutter", ROOT / "analyze_microstutter.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class AnalyzeMicrostutterTests(unittest.TestCase):
    def test_event_window_and_wall_classification(self):
        rows = [{"frame": n, "frame_wall_us": 10000, "event_mask": 0,
                 "fire_native_max_us": 0, "damage_native_max_us": 0} for n in range(1, 11)]
        rows[4]["event_mask"] = 1
        rows[5]["frame_wall_us"] = 23000
        rows[4]["fire_native_max_us"] = 1700
        result = module.analyze(rows)
        self.assertEqual(result["over_budget"]["22222"], 1)
        self.assertEqual(result["events"]["fire"]["count"], 1)
        self.assertEqual(result["events"]["fire"]["window_over_16_7ms"], 1)
        self.assertEqual(result["events"]["fire"]["native_max_us"], 1700)
        self.assertFalse(result["pickup_producer_confirmed"])

    def test_csv_lost_and_first_frame(self):
        csv = ("frame,thread,qpc,frame_wall_us,hook_wall_us,present_call_us,"
               "render_thread_cpu_us,event_mask,fire_native_max_us,damage_native_max_us\n"
               "1,4,100,0,2000,0,1800,0,0,0\n"
               "2,4,200,12000,2000,300,1800,8,0,0\n# lost_frames=2\n")
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "capture.csv"
            path.write_text(csv)
            rows, lost = module.read_capture(path)
        result = module.analyze(rows, lost)
        self.assertEqual(result["frames"], 1)
        self.assertEqual(result["lost_frames"], 2)
        self.assertEqual(result["events"]["use_grip_attempt"]["count"], 1)


if __name__ == "__main__":
    unittest.main()

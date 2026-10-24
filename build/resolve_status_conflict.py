import io
p = "STATUS.md"
t = io.open(p, encoding="utf-8").read()
for m in ("<<<<<<< HEAD\n", "=======\n", ">>>>>>> "):
    while True:
        i = t.find(m)
        if i < 0:
            break
        t = t[:i] + t[i + len(m):]
io.open(p, "w", encoding="utf-8", newline="").write(t)
print("markers removed:", len(t))

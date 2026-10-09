// AUTO-GENERATED from PSK bind skeleton by finger_axes.py - do not edit by hand.
// Flexion axes are unit vectors in BIND space. For each finger the axis was
// verified numerically: rotating the fingertip +25deg about the PIP joint moves
// it toward the palm center. Runtime maps them with the live joint orientation.
#pragma once
namespace fingerpose {
struct FingerJoint { const char* bone; float axis[3]; float maxDeg; };
struct FingerDef { const char* name; int hand; const FingerJoint* joints; int nJoints; };

static const FingerJoint kJoints0[] = {
    {"l_thumb_1", {-0.4793f, 0.0000f, 0.8777f}, 40.0f},
    {"l_thumb_2", {-0.4793f, 0.0000f, 0.8777f}, 45.0f},
    {"l_thumb_3", {-0.4793f, 0.0000f, 0.8777f}, 35.0f},
};
static const FingerJoint kJoints1[] = {
    {"l_index_1", {-0.9971f, 0.0000f, -0.0764f}, 70.0f},
    {"l_index_2", {-0.9971f, 0.0000f, -0.0764f}, 100.0f},
    {"l_index_3", {-0.9971f, 0.0000f, -0.0764f}, 65.0f},
};
static const FingerJoint kJoints2[] = {
    {"l_middle_1", {-0.7260f, 0.0000f, -0.6877f}, 70.0f},
    {"l_middle_2", {-0.7260f, 0.0000f, -0.6877f}, 100.0f},
    {"l_middle_3", {-0.7260f, 0.0000f, -0.6877f}, 65.0f},
};
static const FingerJoint kJoints3[] = {
    {"l_ring_1", {0.9224f, -0.0000f, -0.3863f}, 70.0f},
    {"l_ring_2", {0.9224f, -0.0000f, -0.3863f}, 100.0f},
    {"l_ring_3", {0.9224f, -0.0000f, -0.3863f}, 65.0f},
};
static const FingerJoint kJoints4[] = {
    {"l_pinky_cup", {0.8315f, 0.0000f, 0.5555f}, 25.0f},
    {"l_pinky_1", {0.8315f, 0.0000f, 0.5555f}, 70.0f},
    {"l_pinky_2", {0.8315f, 0.0000f, 0.5555f}, 100.0f},
    {"l_pinky_3", {0.8315f, 0.0000f, 0.5555f}, 65.0f},
};
static const FingerJoint kJoints5[] = {
    {"r_thumb_1", {0.4935f, -0.0000f, 0.8698f}, 40.0f},
    {"r_thumb_2", {0.4935f, -0.0000f, 0.8698f}, 45.0f},
    {"r_thumb_3", {0.4935f, -0.0000f, 0.8698f}, 35.0f},
};
static const FingerJoint kJoints6[] = {
    {"r_index_1", {0.9995f, 0.0000f, 0.0310f}, 70.0f},
    {"r_index_2", {0.9995f, 0.0000f, 0.0310f}, 100.0f},
    {"r_index_3", {0.9995f, 0.0000f, 0.0310f}, 65.0f},
};
static const FingerJoint kJoints7[] = {
    {"r_middle_1", {0.9259f, 0.0000f, -0.3777f}, 70.0f},
    {"r_middle_2", {0.9259f, 0.0000f, -0.3777f}, 100.0f},
    {"r_middle_3", {0.9259f, 0.0000f, -0.3777f}, 65.0f},
};
static const FingerJoint kJoints8[] = {
    {"r_ring_1", {-0.9751f, 0.0000f, -0.2219f}, 70.0f},
    {"r_ring_2", {-0.9751f, 0.0000f, -0.2219f}, 100.0f},
    {"r_ring_3", {-0.9751f, 0.0000f, -0.2219f}, 65.0f},
};
static const FingerJoint kJoints9[] = {
    {"r_pinky_1", {-0.9824f, -0.0000f, 0.1867f}, 70.0f},
    {"r_pinky_2", {-0.9824f, -0.0000f, 0.1867f}, 100.0f},
    {"r_pinky_3", {-0.9824f, -0.0000f, 0.1867f}, 65.0f},
};
static const FingerDef kFingers[] = {
    {"l_thumb", 0, kJoints0, 3},
    {"l_index", 0, kJoints1, 3},
    {"l_middle", 0, kJoints2, 3},
    {"l_ring", 0, kJoints3, 3},
    {"l_pinky", 0, kJoints4, 4},
    {"r_thumb", 1, kJoints5, 3},
    {"r_index", 1, kJoints6, 3},
    {"r_middle", 1, kJoints7, 3},
    {"r_ring", 1, kJoints8, 3},
    {"r_pinky", 1, kJoints9, 3},
};
static const int kFingerCount = sizeof(kFingers) / sizeof(kFingers[0]);
} // namespace fingerpose

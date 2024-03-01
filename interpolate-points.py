from numpy import fft
import math
import matplotlib.pyplot as plt

def lerp(t, a, b):
    return a + (b - a) * t

def normalize(a, d = 1):
    return list(map(lambda p: [p[0]/d, p[1]/d], a))

def scale(a, scale):
    return list(map(lambda p: [p[0]*scale, p[1]*scale], a))

def offset(a, offset):
    return list(map(lambda p: [p[0]+offset, p[1]+offset], a))


def interpolate(a, in_between_points = 100):
    out = []
    for p1, p2 in zip(a, a[1:] + [a[0]]):
        for i in range(in_between_points):
            t = i/in_between_points
            x = lerp(t, p1[0], p2[0])
            y = lerp(t, p1[1], p2[1])
            out.append([x, y])
    return out

def print_signal_array(a, name, type_name="float"):
    out = f"#define {name.upper()}_LENGTH   ({len(a)})\n"
    out += f"{type_name} {name}[{name.upper()}_LENGTH] = {{\n    "

    for i, value in enumerate(a):
        if i > 0 and i % 16 == 0:
            out += "\n    "
        out += f"{value}, "
    out += "\n};"
    print(out)

def plot_fft(signal, sample_rate):
    fft_result = fft.fft(signal)
    Hx = abs(fft_result)
    freqX = fft.fftfreq(len(Hx), 1/sample_rate)

    plt.plot((freqX), Hx) #plot freqX vs Hx
    plt.show()

def plot_signal(signal):
    plt.plot(range(len(signal)), signal)
    plt.show()



interpolation_points = 15
max_scale = 256-1

points = [
    [2, 7],
    [5, 7],
    [6, 6],
    [6, 3],
    [3, 3],
    [2, 4],
    [2, 7],
    [3, 6],
    [6, 6],
    [5, 7],
    [5, 4],
    [6, 3],
    [3, 3],
    [3, 6],
    [3, 3],
    [2, 4],
    [5, 4],
    [5, 7],
]

points = interpolate(points, interpolation_points)

norm_max = 0
for (p,) in zip(points):
    norm_max = max(abs(p[0]), abs(p[1]), norm_max)

normed = normalize(points, norm_max)
scaled = scale(normed, max_scale)
xs = list(map(lambda p: int(p[0] + 0.5), scaled))
ys = list(map(lambda p: int(p[1]+ 0.5), scaled))

# rate = len(xs)
# plot_fft(xs, rate)
# plot_signal(xs * 3)

print(f"#define NUM_POINTS ({len(xs)})")
print_signal_array(xs, "xs", "uint8_t")
print_signal_array(ys, "ys", "uint8_t")
print_signal_array(xs, "xs", "float")
print_signal_array(ys, "ys", "float")
import matplotlib.pyplot as plt
import numpy as np
import math

def linear_to_gamma(linear: float) -> float:
    if linear < 0.0031308:
        return linear * 12.92

    return 1.055 * math.pow(linear, 0.41666) - 0.055

def gamma_to_linear(gamma: float) -> float:
    if gamma < 0.04045:
        return gamma * 0.0773993808

    return math.pow(gamma * 0.9478672986 + 0.0521327014, 2.4)

def main():
    """
    xpoints = np.array([i for i in range(256)])
    ypoints = np.array([gamma_to_linear(i/255) * 255 for i in range(256)])

    plt.plot(xpoints, ypoints)

    plt.grid()
    plt.xlabel("RGB In")
    plt.ylabel("Gamma Out")

    plt.show()
    """
    for i in range(256):
        std_in: float = gamma_to_linear(i/255)
        print(f"{i}: {i == int(round(linear_to_gamma(std_in) * 255))}")

if __name__ == "__main__":
    main()
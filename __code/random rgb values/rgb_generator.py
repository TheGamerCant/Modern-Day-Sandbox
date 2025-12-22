import random

for i in range(30):
    #256 non inclusive

    r: int = random.randrange(0, 256)
    g: int = random.randrange(0, 256)
    b: int = random.randrange(0, 256)

    h = (r << 16) + (g << 8) + b
    
    print(
        f"{str(r)};{str(g)};{str(b)} - {format(h, '#08x')}"
    )

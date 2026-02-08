import random

for i in range(30):
    #256 is non inclusive

    r: int = random.randrange(0, 256)
    g: int = random.randrange(0, 256)
    b: int = random.randrange(0, 256)

    h: str = format(((r << 16) + (g << 8) + b), '#08x')
    
    print(
        f"{str(r)};{str(g)};{str(b)} - {h[2:]}"
    )

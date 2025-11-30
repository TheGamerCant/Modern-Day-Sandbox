while True:
    r : int = int(input("Please enter the r value: "))
    g : int = int(input("Please enter the g value: "))
    b : int = int(input("Please enter the b value: "))
    
    print(f"\n{(r * 256 * 256) + (g * 256) + b}\n\n")

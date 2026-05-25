import socket
import time
import math

SOCKET_PATH = "/tmp/qtest.sock"

PERIODO  = 1.0
MUESTRAS = 20
dt       = PERIODO / MUESTRAS

GPIO_BASE = 0x3F200000
GPSET0    = 0x3F20001C
GPCLR0    = 0x3F200028
GPIO4     = 4
GPIO17    = 17

def enviar_comando(sock, cmd):
    sock.sendall((cmd + "\n").encode())
    return sock.recv(1024).decode().strip()

def set_gpio_analog(sock, pin, valor):
    valor_scaled = int(valor * 0xFFFFFFFF)
    cmd = f"writel 0x{GPIO_BASE + (pin * 4):08x} 0x{valor_scaled:08x}"
    enviar_comando(sock, cmd)

def set_gpio_digital(sock, pin, valor):
    reg = GPSET0 if valor else GPCLR0
    cmd = f"writel 0x{reg:08x} 0x{(1 << pin):08x}"
    enviar_comando(sock, cmd)

def senoidal(t):
    return (math.sin(2 * math.pi * t / PERIODO) + 1) / 2

def cuadrada(t):
    return 1.0 if (t % PERIODO) < (PERIODO / 2) else 0.0

with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
    sock.connect(SOCKET_PATH)
    print("Conectado a qtest.")

    t = 0.0
    try:
        while True:
            set_gpio_analog(sock,  GPIO4,  senoidal(t))
            set_gpio_digital(sock, GPIO17, cuadrada(t))

            print(f"t={t:.2f}s | GPIO4={senoidal(t):.4f} | GPIO17={cuadrada(t):.1f}")

            time.sleep(dt)
            t += dt

    except KeyboardInterrupt:
        print("Señales detenidas.")
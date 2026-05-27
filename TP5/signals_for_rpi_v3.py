# Señales:
#   GPIO4  → Senoidal simulada via PDM (Pulse Density Modulation)
#              Se divide cada período en MUESTRAS sub-muestras.
#              En cada sub-muestra, el pin se pone en HIGH o LOW según
#              si el valor de la senoidal supera un umbral progresivo.
#              La app de usuario promedia las lecturas y reconstruye
#              la forma de onda senoidal.
#
#   GPIO17 → Señal cuadrada digital simple
#              HIGH durante la primera mitad del período,
#              LOW durante la segunda mitad.
#
# Lo que se espera ver en la app graficadora:
#   GPIO4:  Una onda senoidal suavizada entre 0 y 1, con período = PERIODO
#           (reconstruida por promedio de pulsos PDM)
#   GPIO17: Una onda cuadrada perfecta entre 0 y 1, con período = PERIODO

import socket
import time
import math

SOCKET_PATH = "/tmp/qtest.sock"

PERIODO  = 1.0   # segundos por ciclo completo
MUESTRAS = 20    # sub-muestras por período (resolución PDM)
dt       = PERIODO / MUESTRAS

GPSET0 = 0x3F20001C   # registro SET  → pone pin en HIGH
GPCLR0 = 0x3F200028   # registro CLR  → pone pin en LOW

GPIO4  = 4    # señal senoidal PDM
GPIO17 = 17   # señal cuadrada

def enviar_comando(sock, cmd):
    sock.sendall((cmd + "\n").encode())
    return sock.recv(1024).decode().strip()

def set_gpio(sock, pin, valor):
    """Pone el pin GPIO en HIGH (valor=1) o LOW (valor=0)"""
    reg = GPSET0 if valor else GPCLR0
    cmd = f"writel 0x{reg:08x} 0x{(1 << pin):08x}"
    enviar_comando(sock, cmd)

def senoidal_pdm(t, muestra_idx):
    """
    PDM: el pin está en HIGH si el valor de la senoidal
    en este punto supera el umbral progresivo de la sub-muestra.
    Umbral = muestra_idx / MUESTRAS (varía de 0 a 1)
    Resultado: densidad de pulsos HIGH proporcional al valor senoidal.
    """
    valor = (math.sin(2 * math.pi * t / PERIODO) + 1) / 2
    umbral = muestra_idx / MUESTRAS
    return 1 if valor > umbral else 0

def cuadrada(t):
    """HIGH en primera mitad, LOW en segunda mitad del período"""
    return 1 if (t % PERIODO) < (PERIODO / 2) else 0

with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
    sock.connect(SOCKET_PATH)
    print("Conectado a qtest.")
    print(f"Generando señales: GPIO4=senoidal PDM, GPIO17=cuadrada")
    print(f"Período={PERIODO}s, Muestras={MUESTRAS}, dt={dt}s")
    print("-" * 50)

    t = 0.0
    muestra_idx = 0

    try:
        while True:
            val_pdm = senoidal_pdm(t, muestra_idx)
            val_cua = cuadrada(t)

            set_gpio(sock, GPIO4,  val_pdm)
            set_gpio(sock, GPIO17, val_cua)

            print(f"t={t:.2f}s | GPIO4(PDM)={val_pdm} | GPIO17(CUA)={val_cua}")

            time.sleep(dt)
            t += dt
            muestra_idx = (muestra_idx + 1) % MUESTRAS

    except KeyboardInterrupt:
        print("\nSeñales detenidas.")
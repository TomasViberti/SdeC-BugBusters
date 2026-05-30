# =============================================================================
# signals_for_rpi_v5.py
# =============================================================================
# Genera señales GPIO hacia una Raspberry Pi simulada en QEMU.
#
# ARQUITECTURA:
#   Este script actúa como SERVIDOR del socket Unix.
#   Usa la misma estructura de VGPIOManager (qemu-rpi-gpio):
#     1. Crea el socket UNIX-LISTEN en /tmp/tmp-gpio.sock via socat
#     2. Espera (bloqueante) a que QEMU se conecte como cliente
#     3. Una vez conectado, envía comandos writel para controlar los GPIOs
#
#   Orden de arranque:
#     Terminal 1 → python3 signals_for_rpi_v5.py   (crea el socket y espera)
#     Terminal 2 → ./run.sh                          (QEMU arranca y conecta)
#
# SEÑALES:
#   GPIO4  → Senoidal via PDM (Pulse Density Modulation)
#              Densidad de pulsos HIGH proporcional al valor senoidal.
#              Reconstruible por promedio de lecturas.
#   GPIO17 → Cuadrada clásica (HIGH primera mitad, LOW segunda mitad)
#
# REQUISITOS:
#   - socat instalado:   sudo apt install socat
#   - pexpect instalado: pip install pexpect
#   - run.sh con:        ENABLEQTEST=true  y  QTESTSOCKET="/tmp/tmp-gpio.sock"
# =============================================================================

import sys
import time
import math
import os
import pexpect


# =============================================================================
# Configuración
# =============================================================================

SOCK_PATH   = "/tmp/tmp-gpio.sock"

# Registros del BCM2835/BCM2837 (Raspberry Pi 3, usado por raspi3b en QEMU)
GPIO_BASE        = 0x3f200000
GPIO_SET_OFFSET  = 0x1c
GPIO_RESET_OFFSET= 0x28

# Parámetros de señal
PERIODO  = 1.0    # segundos por ciclo completo
MUESTRAS = 20    # sub-muestras por período (resolución PDM)
                  # Con sigma-delta, más muestras → reconstrucción más fiel.
                  # 100 muestras a 1s de período = 1 bit cada 10ms.
                  # En app.py usar MUESTRAS_PDM = 100 y READ_PERIOD_S = 0.01
dt       = PERIODO / MUESTRAS

GPIO4  = 4        # señal senoidal PDM
GPIO17 = 17       # señal cuadrada


# =============================================================================
# Comunicación con QEMU (misma estructura que VGPIOManager en qemu-rpi-gpio)
# =============================================================================

class GPIOServer:
    """
    Servidor de socket Unix que habla el protocolo qtest de QEMU.
    Replica la estructura de VGPIOManager pero orientado a envío
    continuo de señales en lugar de comandos interactivos.
    """

    def __init__(self, sock_path: str = SOCK_PATH):
        self.sock_path = sock_path
        self._conectar()

    def _conectar(self):
        # Si el socket existe de una sesión anterior, lo eliminamos
        if os.path.exists(self.sock_path):
            os.unlink(self.sock_path)
            print(f"[INFO] Socket anterior eliminado: {self.sock_path}")

        print(f"[...] Esperando conexión de QEMU en '{self.sock_path}'...")
        print(f"      Ahora podés correr:  ./run.sh")
        print(f"      (bloqueante hasta que QEMU conecte)\n")

        # socat conecta stdin/stdout al socket Unix como servidor
        # Idéntico a lo que hace VGPIOManager.load() en qemu-rpi-gpio
        self.fd = pexpect.spawn(f"socat - UNIX-LISTEN:{self.sock_path}")
        print("[OK] QEMU conectado. Iniciando señales.\n")

    def _sendline(self, s: str):
        self.fd.sendline(s)

    def _read(self):
        self.fd.readline()   # cancelar echo
        return self.fd.readline()

    def writel(self, address: int, value: int):
        """Escribe 32 bits en una dirección de memoria mapeada de QEMU."""
        self._sendline(f"writel 0x{address:x} 0x{value:x}")
        return self._read()

    def set_gpio(self, pin: int, value: int):
        """
        Pone un pin GPIO en HIGH (value=1) o LOW (value=0).
        Calcula la dirección del registro SET o RESET del BCM2837
        y la máscara de bit correspondiente al pin.
        """
        base   = GPIO_BASE + int(pin / 32) * 4
        offset = GPIO_SET_OFFSET if value else GPIO_RESET_OFFSET
        mascara = 1 << (pin % 32)
        self.writel(base + offset, mascara)

    def close(self):
        self.fd.close()


# =============================================================================
# Formas de onda
# =============================================================================

class SigmaDelta:
    """
    Modulador sigma-delta de primer orden para PDM.

    En cada llamada a next(valor):
      1. Se suma el valor de entrada [0,1] al acumulador de error.
      2. Si acumulador >= 0.5 → emite 1 y resta 1.0 al acumulador.
         Si acumulador <  0.5 → emite 0.

    La densidad de 1s en la salida es proporcional al valor de entrada,
    y el error de cada muestra se recupera en la siguiente. Esto elimina
    la distorsión triangular del método de umbral lineal fijo.
    """
    def __init__(self):
        self.acumulador = 0.0

    def next(self, valor: float) -> int:
        """valor debe estar en [0, 1]."""
        self.acumulador += valor
        if self.acumulador >= 0.5:
            self.acumulador -= 1.0
            return 1
        return 0


def valor_seno(t: float) -> float:
    """Senoidal normalizada a [0, 1]."""
    return (math.sin(2 * math.pi * t / PERIODO) + 1) / 2


def cuadrada(t: float) -> int:
    """HIGH en la primera mitad del período, LOW en la segunda."""
    return 1 if (t % PERIODO) < (PERIODO / 2) else 0


# =============================================================================
# Main
# =============================================================================

def main():
    print("=" * 55)
    print(" signals_for_rpi_v5.py — GPIO signals via qtest socket")
    print("=" * 55)
    print(f"  Socket  : {SOCK_PATH}")
    print(f"  Período : {PERIODO}s  |  Muestras: {MUESTRAS}  |  dt: {dt}s")
    print(f"  GPIO4   : senoidal PDM (sigma-delta)")
    print(f"  GPIO17  : cuadrada")
    print("-" * 55)

    try:
        servidor = GPIOServer(SOCK_PATH)
    except Exception as e:
        print(f"[ERROR] No se pudo iniciar el servidor GPIO: {e}")
        print("        ¿Está socat instalado? → sudo apt install socat")
        sys.exit(1)

    sd  = SigmaDelta()
    t   = 0.0

    try:
        while True:
            val_pdm = sd.next(valor_seno(t))
            val_cua = cuadrada(t)

            servidor.set_gpio(GPIO4,  val_pdm)
            servidor.set_gpio(GPIO17, val_cua)

            print(f"t={t:6.3f}s | GPIO4 (PDM)={val_pdm} | GPIO17 (CUA)={val_cua}")

            time.sleep(dt)
            t += dt

    except KeyboardInterrupt:
        print("\n[INFO] Señales detenidas por el usuario.")

    finally:
        servidor.set_gpio(GPIO4,  0)
        servidor.set_gpio(GPIO17, 0)
        servidor.close()
        print("[INFO] Servidor cerrado. Pines en LOW.")


if __name__ == "__main__":
    main()

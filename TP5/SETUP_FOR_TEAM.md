# Guía completa: Migración a ARM64, driver `gpiod`, app Python y QEMU GPIO

Este documento explica paso a paso cómo reproducir el entorno que desarrollamos. Incluye la limpieza de una instalación previa ("arquitectura vieja"), la instalación/configuración de la nueva imagen ARM64, cómo compilar y cargar el driver que usa gpiod, cómo ejecutar la app Python y cómo usar el simulador `qemu-rpi-gpio` para forzar GPIOs desde el host.

Importante: cada bloque indica claramente dónde ejecutar los comandos —Host (tu máquina de desarrollo) o Guest (la Raspberry virtual via QEMU, accesible por SSH en `127.0.0.1:50022`). Sustituí el usuario y rutas si tu entorno difiere.

---

## Resumen de archivos relevantes (en el repo)
- `TP5/app.py` — aplicación Python (GUI) que lee `/dev/sensor_driver` y dibuja tiempo vs valor.
- `TP5/CDD/sensor_driver.c` — driver de kernel character device que usa gpiod/gpiochip para leer GPIO4/GPIO17.
- `TP5/qemu-rpi-gpio/qemu-rpi-gpio` — simulador de GPIO que habla con QEMU por `qtest`.
- `TP5/qemu-rpi-gpio/qemu-pi-setup/run.sh` — script para arrancar QEMU con parámetros (RAM/CPUs, qtest socket, port forward SSH).
- `TP5/SETUP_FOR_TEAM.md` — este documento.

---

# 0. Recomendación previa
- Haz backup de cualquier imagen o VM importante antes de borrar archivos.
- Esta guía asume que trabajás desde el repo en `~/Documentos/SIstemas de Computación/SdeC-BugBusters`.

---

# 1. LIMPIAR "arquitectura vieja" (Host)
Si hay instalaciones previas (imágenes antiguas, rootfs viejas, sockets, procesos qemu antiguos), limpiá para evitar conflictos.

Advertencia: los comandos de borrado eliminan archivos locales — comprobá rutas.

1.1 Parar procesos QEMU y simulador (Host):
```bash
# listar procesos qemu/rvgpio
ps aux | egrep 'qemu|qemu-system' | egrep -v 'egrep' || true
# matar procesos qemu (si existen)
sudo pkill -9 qemu-system-aarch64 || true
# cerrar simulador qemu-rpi-gpio (si arrancó en background)
pkill -f qemu-rpi-gpio || true
```

1.2 Eliminar imágenes/archivos temporales del repo (Host):
```bash
# borrar imágenes / rootfs generadas por setup (si no las necesitás)
rm -f ~/Documentos/SIstemas\ de\ Computación/SdeC-BugBusters/TP5/qemu-rpi-gpio/qemu-pi-setup/raspios_lite_armhf_latest.img*
rm -rf ~/Documentos/SIstemas\ de\ Computación/SdeC-BugBusters/TP5/qemu-rpi-gpio/qemu-pi-setup/rootfs* || true
# eliminar socket temporal del simulador
rm -f /tmp/tmp-gpio.sock /tmp/qtest.sock || true
```

1.3 (Opcional) Desinstalar paquetes QEMU antiguos (Host) — solo si querés purgar:
```bash
# Opcional: purgar qemu si vas a reinstalar
sudo apt remove --purge -y qemu-system-arm qemu-system-aarch64 qemu
sudo apt autoremove -y
```

Verificación (Host):
```bash
# no deben quedar procesos qemu ni sockets en /tmp
ps aux | egrep 'qemu-system' || true
ls -l /tmp | egrep 'tmp-gpio|qtest' || true
```

---

# 2. Instalar nuevo entorno ARM64 (Host)
Esto prepara la imagen ARM64 y QEMU. Ejecutar en Host.

2.1 Instalar dependencias en Host:
```bash
sudo apt update
sudo apt install -y qemu-system-aarch64 socat python3-pexpect xz-utils python3-pip
# herramientas libgpiod en host (útiles para debug local)
sudo apt install -y libgpiod2 libgpiod-dev gpiod libgpiod-tools
```

2.2 Descargar y preparar la imagen ARM64 (usando el script del repo):
```bash
cd ~/Documentos/SIstemas\ de\ Computación/SdeC-BugBusters/TP5/qemu-rpi-gpio/qemu-pi-setup
# El script descarga y prepara la imagen arm64
./setup_arm64.sh
# si falla revisar salida y deps (wget, unxz)
```

2.3 Verificar que la imagen esté donde esperás (Host):
```bash
ls -lh ~/Documentos/SIstemas\ de\ Computación/SdeC-BugBusters/TP5/qemu-rpi-gpio/qemu-pi-setup/raspios_lite_armhf_latest.img
```

2.4 Configurar recursos del VM (editar `run.sh` si querés otros valores):
```bash
# Editar si querés RAM/CPUs distintos (nota: raspi3b acepta 1024 MiB y hasta 4 CPUs)
nano ~/Documentos/SIstemas\ de\ Computación/SdeC-BugBusters/TP5/qemu-rpi-gpio/qemu-pi-setup/run.sh
# valores importantes dentro: MEMORY_MB=1024  SMP_CPUS=4
```

Verificación sintáctica (Host):
```bash
bash -n ~/Documentos/SIstemas\ de\ Computación/SdeC-BugBusters/TP5/qemu-rpi-gpio/qemu-pi-setup/run.sh
```

2.5 Iniciar simulador GPIO y la VM (Host):
Abrir dos terminales:
- Terminal A (simulador):
```bash
cd ~/Documentos/SIstemas\ de\ Computación/SdeC-BugBusters/TP5/qemu-rpi-gpio
./qemu-rpi-gpio
# verás prompt: (gpio)>
```
- Terminal B (arranque VM):
```bash
cd ~/Documentos/SIstemas\ de\ Computación/SdeC-BugBusters/TP5/qemu-rpi-gpio/qemu-pi-setup
./run.sh
```

Verificación: en la Terminal B no debe aparecer error de SMP o RAM (si aparece, ajustá `run.sh`).

---

# 3. Preparar el Guest (Guest / Raspberry virtual)
Con la VM en marcha, conectate por SSH desde el Host (puerto 50022):

```bash
ssh -p 50022 tomas@127.0.0.1 ## chequear el usuario que hayan puesto en su rsbp
```

3.1 Instalar dependencias en el Guest (compilación y Python GUI):
```bash
sudo apt update
sudo apt install -y build-essential git dkms
# headers: raspbian/raspios provee paquetes; probar la variante del sistema
sudo apt install -y raspberrypi-kernel-headers || sudo apt install -y linux-headers-$(uname -r)

# Python + GUI
sudo apt install -y python3 python3-pip python3-tk python3-matplotlib

# libgpiod (user-space) y herramientas
sudo apt install -y libgpiod2 libgpiod-dev libgpiod-tools gpiod
```

Verificación (Guest):
```bash
gcc --version
python3 -c "import tkinter; import matplotlib; print('TK+MATPLOTLIB OK')"
which gpioset || echo 'gpioset no encontrado'
```

---

# 4. Sincronizar el repo y compilar el driver (Host → Guest)

4.1 Desde Host: copiar archivos modificados (ejemplo: `app.py`, `sensor_driver.c`, `run.sh` si corresponde). Esto es para actualizar los archivos dentro de la raspberry. Hacerlo si o si luego de hacer el git pull.
```bash
# copiar app.py
scp -P 50022 "~/Documentos/SIstemas de Computación/SdeC-BugBusters/TP5/app.py" tomas@127.0.0.1:~/TP5/app.py

# copiar driver
scp -P 50022 "~/Documentos/SIstemas de Computación/SdeC-BugBusters/TP5/CDD/sensor_driver.c" tomas@127.0.0.1:~/TP5/CDD/sensor_driver.c

# copiar run.sh
scp -P 50022 "~/Documentos/SIstemas de Computación/SdeC-BugBusters/TP5/qemu-rpi-gpio/qemu-pi-setup/run.sh" tomas@127.0.0.1:~/TP5/qemu-rpi-gpio/qemu-pi-setup/run.sh
```

4.2 En el Guest: compilar y cargar el módulo
```bash
ssh -p 50022 tomas@127.0.0.1
cd ~/TP5/CDD
make clean
make
# o (método esperado por el Makefile del repositorio):
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

# cargar el módulo (Makefile opcionalmente incluye target 'load')
sudo make load

# Verificar logs
sudo dmesg | tail -30
lsmod | grep sensor_driver || true
ls -l /dev/sensor_driver
```

Salida esperada: mensajes del driver en dmesg, `/dev/sensor_driver` creado.

---

# 5. ¿Qué cambió en el driver? (explicación técnica breve)
- Motivo: en sistemas modernos y en emulación, escribir/usar `/sys/class/gpio/export` puede fallar o no estar implementado. La API de descriptors `gpiod` (kernel GPIO descriptor API) es la recomendada.
- Cambios realizados en `sensor_driver.c`:
  - Uso de `gpiochip_request_own_desc()` para reservar descriptores por chip y offset, con `GPIO_LOOKUP_FLAGS_DEFAULT` y `GPIOD_IN`.
  - Lectura con `gpiod_get_value_cansleep()` en lugar de leer `sysfs`.
  - Manejo correcto de `IS_ERR_OR_NULL()` y limpieza (`gpiochip_free_own_desc()`, `gpio_device_put()`).
  - Registro de `char device` que expone lecturas como líneas ASCII: `GPIO4:0` o `GPIO17:1`.

Verificación del cambio (Guest):
```bash
# luego de cargar módulo
gpioinfo
# líneas deben mostrar consumer="sensor_driver_gpio4" y consumer="sensor_driver_gpio17"
```

---

# 6. Probar lectura desde el driver (Guest)
1. Seleccionar qué pin quiere leer la app (0 -> GPIO4, 1 -> GPIO17):
```bash
echo 0 | sudo tee /dev/sensor_driver
cat /dev/sensor_driver
# salida: GPIO4:0 (o 1)
```

2. Si el módulo no devolviera valor, revisar `dmesg`:
```bash
sudo dmesg | tail -50
```

---

# 7. Forzar valores GPIO desde el Host con `qemu-rpi-gpio` (Host)
En la terminal del simulador (donde ejecutaste `./qemu-rpi-gpio`) y de a una linea por vez: 
```text
(gpio)> set 4 1     # fuerza GPIO4 a 1
(gpio)> set 4 0     # fuerza GPIO4 a 0
(gpio)> toggle 4    # alterna
(gpio)> get 4       # lee el valor actual
```

Comprobá en el Guest:
```bash
cat /dev/sensor_driver
# debe mostrar el valor acordado
```

Notas: si el driver ha reclamado la línea, `gpioset` en Guest no podrá tocar la línea, por eso es necesario usar el simulador del Host o liberar la línea (`sudo rmmod sensor_driver`).

---

# 8. Ejecutar la app Python (Guest o Host)

8.1 Desde Guest (más directo, pero a veces lento en QEMU) como dice la consigna: 
```bash
ssh -p 50022 tomas@127.0.0.1 ## chequear el nombre de usuario o login
cd ~/TP5
python3 app.py
```

8.2 Desde Host (recomendado por rendimiento):
- Ejecutar `app.py` en el host mejora el rendimiento gráfico. Para que funcione, `app.py` debe poder leer un endpoint que exponga el valor; el driver char que está dentro de la VM no estará disponible directamente en el host. Opciones:
  - Ejecutar una pequeña utilidad en Guest que publique la lectura por TCP y el host consuma ese TCP.
  - O usar `ssh` para ejecutar `cat /dev/sensor_driver` periódicamente y pasarlo al host (menos ideal).

8.3 Verificaciones dentro de la app
- Al pulsar `Ver GPIO4`, la app debe enviar `0` al driver y empezar a graficar tiempo vs valor.
- Si la app tarda, reducir la frecuencia de refresco en `app.py` (constante `READ_PERIOD_S`).

---

# 9. Limpieza final y puntos a documentar para el equipo
- Documentar en el repo lo que se cambió: añadir `SETUP_FOR_TEAM.md` (este archivo) y comentarlo en el README principal.
- Indicar en el README del proyecto que la GUI se aconseja ejecutar fuera de la VM para mejor rendimiento.
- Añadir breve script `sync_and_restart.sh` (opcional) para sincronizar y recargar driver.

---

# 10. Copiar esencial (chequeo rápido)
En Host, ejecutar estos pasos en orden para validar rápidamente (resumen):
```bash
# 1. Parar viejos procesos
sudo pkill -9 qemu-system-aarch64 || true
pkill -f qemu-rpi-gpio || true
rm -f /tmp/tmp-gpio.sock /tmp/qtest.sock || true

# 2. Levantar simulador y VM
cd ~/Documentos/SIstemas\ de\ Computación/SdeC-BugBusters/TP5/qemu-rpi-gpio
./qemu-rpi-gpio &
cd ~/Documentos/SIstemas\ de\ Computación/SdeC-BugBusters/TP5/qemu-rpi-gpio/qemu-pi-setup
./run.sh

# 3. Conectar por SSH al Guest
ssh -p 50022 tomas@127.0.0.1
# 4. En Guest: compilar y cargar driver
cd ~/TP5/CDD
make clean && make
sudo make load
sudo dmesg | tail -n 20
# 5. En Host (simulador): forzar gpio
# en la terminal del simulador: (gpio)> set 4 1
# 6. En Guest: leer
cat /dev/sensor_driver
```

---

Si querés, ahora puedo:
- Generar el `sync_and_restart.sh` que copie `app.py` y `sensor_driver.c` al Guest y ejecute `make load` automáticamente. (Puedo generarlo y validarlo sintácticamente.)
- Añadir una sección corta sobre cómo implementar un pequeño proxy TCP en el Guest para exponer lecturas al Host (si preferís ejecutar la GUI en el Host).

Dime cuál querés que haga ahora y lo genero (README ya creado en `TP5/SETUP_FOR_TEAM.md`).

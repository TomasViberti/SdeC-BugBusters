---

<div align="center">

# Universidad Nacional de Córdoba
## Facultad de Ciencias Exactas, Físicas y Naturales

---

### Sistemas de Computación
# Trabajo Práctico N°5
## Linux Kernel Module Programming II

**Grupo:** BugBusters

| Integrantes |
|:----------:|
| Alfici, Facundo |
| Capdevila, Gastón |
| Viberti, Tomás |

| Docentes |
|:---:|
| Jorge, Javier Alejandro |
| Solinas, Miguel |

**2026**

</div>

---

## Índice

1. [Introducción](#introducción)
2. [¿Qué es un driver?](#qué-es-un-driver)
3. [Tipos de drivers en Linux](#tipos-de-drivers-en-linux)
4. [Character Device Driver (CDD)](#character-device-driver-cdd)
5. [Entorno de desarrollo](#entorno-de-desarrollo)
6. [Ejemplos de la cátedra](#ejemplos-de-la-cátedra)
   - [drv1](#drv1-módulo-básico)
   - [drv2](#drv2-registro-de-majorminor)
   - [drv3](#drv3-cdd-completo-con-dev)
   - [drv4](#drv4-readwrite-con-espacio-de-usuario)
7. [CDD](#cdd)
8. [Aplicación de usuario](#aplicación-de-usuario) 
9. [Conclusiones](#conclusiones) 

---

## Introducción

Este trabajo práctico aborda la programación de módulos del kernel de Linux, con foco en la construcción de un **Character Device Driver (CDD)**. El objetivo final es desarrollar un driver que permita sensar dos señales GPIO externas con un período de un segundo, y una aplicación de usuario que lea una de las señales y la grafique en tiempo real.

El desarrollo se realizó sobre una Raspberry Pi virtualizada con QEMU, cargando un módulo del kernel que expone el dispositivo `/dev/sensor_driver` y una app Python que interactúa con ese nodo desde espacio de usuario.

---

## ¿Qué es un driver?

Un **driver** es un software que permite al sistema operativo interactuar con un periférico. Crea una abstracción del hardware y proporciona una interfaz estandarizada para utilizarlo.

Se distinguen dos conceptos relacionados:

- **Device driver**: pieza de software que controla un dispositivo físico.
- **Device controller**: otro periférico que controla dispositivos, y que a su vez necesita su propio driver (llamado *bus driver*).

Todo device driver tiene dos partes:
1. Una parte **específica del dispositivo** (qué hace el hardware).
2. Una parte **específica del sistema operativo** (cómo se comunica con el kernel).

---

## Tipos de drivers en Linux

Basándose en la interfaz específica del sistema operativo, Linux clasifica los drivers en tres categorías verticales:

| Tipo | Orientación | Ejemplos |
|------|------------|---------|
| **Network** | Paquetes | Ethernet, WiFi |
| **Block** | Bloques | Discos, SSDs |
| **Character** | Bytes | Serial, audio, video, GPIO |

El directorio `/dev` contiene los archivos de dispositivo especiales para todos los dispositivos. Los character devices se pueden listar con:

```bash
ls -l /dev/ | grep "^c"
cat /proc/devices
```

---

## Character Device Driver (CDD)

El CDD es el tipo más común de driver en Linux. Todo driver que no es de almacenamiento ni de red es algún tipo de CDD. Ejemplos: puertos seriales, audio, video, cámaras, GPIO.

### Modelo de capas

```
[ Aplicación de usuario ]
        ↕  (nombre de archivo)
[ Character Device File - CDF ]  →  /dev/mi_dispositivo
        ↕  (número major/minor)
[ Character Device Driver - CDD ]  →  módulo del kernel
        ↕
[ Hardware / señal simulada ]
```

El vínculo entre la aplicación y el CDF se basa en el **nombre** del archivo. El vínculo entre el CDF y el CDD se basa en el **número** `<major, minor>`, no en el nombre.

### Major y Minor numbers

```bash
# Ver dispositivos de caracteres registrados
cat /proc/devices

# Ejemplo de salida:
# 239 SdeC_drv3
```

- **Major number**: identifica el driver.
- **Minor number**: identifica el dispositivo específico dentro del driver.

En el kernel, el tipo `dev_t` (32 bits: 12 para major, 20 para minor) contiene ambos. Las macros útiles son:

```c
MAJOR(dev_t dev)               // extrae el major
MINOR(dev_t dev)               // extrae el minor
MKDEV(int major, int minor)    // construye un dev_t
```

---

## Entorno de desarrollo

### Verificación del entorno

```bash
uname -r                                    # versión del kernel
ls /usr/src/linux-headers-$(uname -r)       # headers instalados
gcc --version
make --version
```

### Instalación de dependencias

```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r) gcc make
```
---

## Ejemplos de la cátedra

### drv1 Módulo básico

El módulo más simple posible. Solo demuestra el ciclo de vida de un módulo del kernel: carga (`insmod`) y descarga (`rmmod`).

**Conceptos clave:**
- `module_init()` / `module_exit()` — registran el constructor y destructor.
- `printk()` equivalente a `printf` pero en espacio del kernel, escribe en el log del kernel.
- `MODULE_LICENSE("GPL")` — obligatorio para evitar que el kernel quede marcado como "tainted".

**Prueba:**

<img width="1157" height="626" alt="image" src="https://github.com/user-attachments/assets/2622bbb3-8c54-4f4b-916c-08223c2a6cf3" />

---

### drv2 Registro de major/minor

Agrega el registro dinámico de números de dispositivo usando `alloc_chrdev_region()`.

**Conceptos clave:**
- `alloc_chrdev_region()` el kernel asigna automáticamente un major libre.
- `unregister_chrdev_region()` libera los números al descargar.
- Los minor numbers se pueden ver creando archivos de dispositivo manualmente con `mknod`.

**Prueba:**

<img width="1109" height="673" alt="image" src="https://github.com/user-attachments/assets/23e826d0-63fb-41d0-9f5a-f10974c103bc" />

---

### drv3 CDD completo con /dev

Primer driver que crea automáticamente el archivo `/dev/SdeC_drv3` usando `udev`.

**Conceptos clave:**
- `class_create()` crea una clase de dispositivo en `/sys/class/`.
- `device_create()` crea la entrada en `/sys/class/` que `udev` convierte en `/dev/SdeC_drv3`.
- `cdev_init()` + `cdev_add()` vincula las `file_operations` al CDD.
- `file_operations` estructura que mapea operaciones de archivo (`open`, `read`, `write`, `release`) a funciones del driver.

**Prueba:**

<img width="1223" height="895" alt="image" src="https://github.com/user-attachments/assets/fc957f5f-87f3-4a30-b813-4701890fa59e" />

En drv3, `read()` devuelve 0 (EOF) y `write()` no hace nada con los datos solo registra que fue llamado.

---

### drv4 read/write con espacio de usuario

Extiende drv3 implementando transferencia real de datos entre kernel y usuario.

**Conceptos clave:**
- `copy_to_user(buf, &c, 1)` copia datos del kernel al espacio de usuario (para `read`).
- `copy_from_user(&c, buf, 1)` copia datos del espacio de usuario al kernel (para `write`).
- El offset `*off` se usa para indicar EOF en lecturas sucesivas.
- drv4 guarda **un solo byte** `c` que se puede escribir y leer.

**Por qué no se puede acceder directo a punteros de usuario:**  
El kernel y el espacio de usuario tienen espacios de memoria separados y protegidos. Las funciones `copy_to_user` / `copy_from_user` hacen la transferencia de forma segura, verificando que los punteros sean válidos.

**Prueba:**

<img width="1206" height="895" alt="image" src="https://github.com/user-attachments/assets/493b17a7-eb81-4918-89a4-3bfe618e34fa" />

---

## Generación de Señales

En este apartado, se realizóun Script de Python que se encargará de generar y enviar dos señales por los puertos 4 y 17 del GPIO de la Raspberry PI. La idea es que las señales tengan los siguientes parámetros y características:

- Señal senoidal por el puerto GPIO4 con período de 1s
- Señal cuadrada por el puerto GPIO17 con periodo de 1s

Una vez definido esto, debemos tomar en cuenta algunas consideraciones. En primera medida, es importante aclarar que los puertos de porpósito general de la RPI pueden tomar solamente valores de 0 o 1, es decir, son **digitales**. Esta característica nos limita en gran medida a cumplir con la consigna, pero existe un método para poder alcanzar el objetivo. Para reconstruir el valor original de la señal codificada es necesario promediar N bits consecutivos.

El script final utilizado para la generación de señales se llama `scripts_for_rpi_v5.py`. Las versiones anteriores del mismo fueron pruebas que fracasaron con el cumplimiento de la consigna, pero para demostrar el trabajo realizado siguen disponibles.

### Arquitectura de comunicación

El script actúa como **servidor** de un socket Unix. Cuando arranca, crea el socket en `/tmp/tmp-gpio.sock` y espera. Al correr `run.sh`, QEMU arranca y se conecta a ese socket como cliente. A partir de ahí, el script le envía comandos del protocolo **qtest** de QEMU, que son escrituras directas en los registros de memoria del chip BCM2837 (el GPIO controller de la Raspberry Pi 3).

### Señal PDM

Es una forma de representar un valor analógico continuo usando únicamente bits 0 y 1, donde la información no está en el ancho de los pulsos sino en la densidad, es decir, cuántos 1's hay en una ventana de tiempo determinada. Esta forma es la que usaremos para representar la onda senoidal.

Para la onda cuadrada, el proceso de generación es más sencillo, simplemente se hará un toggle entre 0 y 1 con período de 1s.

### Consideraciones técnicas

Algunas consideraciones a tener en cuenta en lo que respecta a la generación de señales es que, en lo que refiere a la cantidad de muestras por período de la señal analógica (senoidal), se debe utilizar un valor lo suficientemente alto como para que la señal sea lo mas fiel posible a la representación real de la misma. Ahora, sabiendo esto, las limitaciones de cómputo del emulador QEMU no nos permiten una cantidad de muestras mayores a 400, por lo que el período de la señal deberá de ser 4s como mínimo para una representación aceptable.

### Orden de arranque

1. `python3 signals_for_rpi_v5.py` → crea el socket y bloquea esperando
2. `./run.sh` → QEMU arranca y conecta al socket
3. Las señales empiezan a fluir automáticamente

Los valores enviados se imprimirán en la terminal en el que se ejecute el comando, como se ve a continuación.

<img width="452" height="402" alt="imagen" src="https://github.com/user-attachments/assets/ac0428de-bdec-4d66-a73b-f34b2f3242e8" />

---

## CDD

El driver desarrollado actúa como puente entre la aplicación de usuario y el kernel. Su responsabilidad es exponer un nodo de caracteres en `/dev`, seleccionar cuál de las dos señales GPIO se va a leer y devolver el valor leído en un formato simple de texto.

### Flujo general del sistema

```
[ App Python ]
        |
        | write("0") o write("1")
        v
[ /dev/sensor_driver ]
        |
        | file_operations -> write/read
        v
[ sensor_driver.c ]
        |
        | API de GPIO del kernel
        v
[ gpiochip / gpio_desc / gpiod_get_value_cansleep() ]
        |
        v
[ Kernel + controlador GPIO + hardware GPIO ]
```
Para iniciar el sistema:
<img width="947" height="265" alt="image" src="https://github.com/user-attachments/assets/662b8462-f557-4801-b2e0-3a916901d11c" />

<img width="1493" height="460" alt="image" src="https://github.com/user-attachments/assets/78a5ef29-ad61-499b-b211-ecd9e4022c94" />

<img width="954" height="290" alt="image" src="https://github.com/user-attachments/assets/df9f48ec-fd9a-4511-b829-beb2a0c845c7" />

<img width="950" height="554" alt="image" src="https://github.com/user-attachments/assets/c56c8154-5bff-4953-b17f-8bf7bab5b804" />

### Cómo se obtiene el dato del GPIO

Al cargar el módulo, el driver busca el `gpio_device` disponible y obtiene el `gpio_chip` asociado. Después reserva dos descriptores de GPIO con la API de descriptores del kernel:

- `GPIO4` como señal 0.
- `GPIO17` como señal 1.

La lectura no se hace mediante sysfs, sino con `gpiod_get_value_cansleep()`. Esa función consulta el estado lógico del descriptor GPIO de forma segura, respetando las reglas del kernel y permitiendo su uso en contextos donde la lectura puede dormir.

Cuando la aplicación hace una lectura sobre `/dev/sensor_driver`, el driver devuelve una línea de texto con este formato:

```text
GPIO4:0
GPIO17:1
```

### Interacción con el kernel

El módulo registra un `character device` con `alloc_chrdev_region()`, `class_create()`, `device_create()` y `cdev_add()`. Esa cadena hace que el kernel cree el nodo en `/dev` y conecte las operaciones de archivo del módulo con la aplicación de usuario.

Las operaciones implementadas son:

- `open()` y `release()` para registrar el acceso al dispositivo.
- `write()` para seleccionar qué GPIO se va a monitorear.
- `read()` para consultar el valor actual del GPIO elegido.

El `write()` interpreta un byte:

- `0` selecciona `GPIO4`.
- `1` selecciona `GPIO17`.

Luego `read()` arma la respuesta textual y la copia al espacio de usuario con `copy_to_user()`. De este modo, la aplicación no accede al hardware directamente, sino a través del contrato que define el driver.

### Criterio de diseño

El uso de la API moderna de GPIO del kernel evita depender de mecanismos antiguos como sysfs. Además, permite que el driver tenga una interfaz limpia y centrada en descriptores, con mejor integración con la infraestructura interna del kernel.

### Prueba realizada

La validación del CDD consistió en cargar el módulo, verificar la creación de `/dev/sensor_driver`, escribir la selección de GPIO desde la aplicación y confirmar que la lectura devolviera el valor esperado en el gráfico.

---

## Aplicación de usuario

La aplicación de usuario está escrita en Python y usa `tkinter` para la interfaz y `matplotlib` para la visualización en tiempo real. Su función es escribir la selección del GPIO sobre `/dev/sensor_driver`, leer una vez por segundo el valor publicado por el driver y mostrarlo en un gráfico temporal.

### Flujo de la app

1. El usuario presiona `Ver GPIO4` o `Ver GPIO17`.
2. La app escribe `0` o `1` en `/dev/sensor_driver`.
3. El driver cambia el GPIO monitoreado internamente.
4. Un hilo de lectura consulta el dispositivo cada segundo.
5. La respuesta textual del driver se parsea con una expresión regular.
6. El valor se agrega a las listas de tiempo y muestras.
7. `matplotlib` actualiza el gráfico embebido.

Como prueba, se muestra la inicializacion de la app desde la propia RaspberryPi simulada en Qemu:

<img width="950" height="95" alt="image" src="https://github.com/user-attachments/assets/9b186d9b-af64-4780-975a-7e0bd38dcc65" />

En este recorrido se ve también el cambio de señal en la interfaz cuando se alterna entre GPIO4 y GPIO17. En primera instancia se selecciona el GPIO4:

<img width="999" height="737" alt="image" src="https://github.com/user-attachments/assets/8c2847b6-8e66-4806-9cc7-e16a10761e3c" />


Luego cambiamos a GPIO17:

<img width="997" height="735" alt="image" src="https://github.com/user-attachments/assets/91495116-20c7-4c74-a8ec-9e746ee50c71" />

En la consola de la RaspberryPi:

<img width="950" height="134" alt="image" src="https://github.com/user-attachments/assets/bce56b9c-80c8-46f8-b6eb-c94b2cce0be4" />

Al setear un 1 en el GPIO17 desde el emulador:

<img width="959" height="330" alt="image" src="https://github.com/user-attachments/assets/b8491914-2402-410d-a672-9f4530840ee5" />

Vemos como se actualiza el gráfico:

<img width="1004" height="744" alt="image" src="https://github.com/user-attachments/assets/b6c22aad-5298-4434-b1a5-dd64bc7c0c7b" />

### Detalles de implementación

La app usa un hilo secundario para evitar bloquear la interfaz gráfica. Ese hilo:

- espera a que haya un GPIO seleccionado,
- abre el dispositivo en modo lectura,
- obtiene una línea de texto,
- la interpreta con un patrón del tipo `GPIO<numero>:<valor>`,
- y almacena la muestra en memoria para graficarla.

- Cumpliendo con los requerimientos específicos del enunciado, la visualización invierte los ejes convencionales: el valor lógico de la señal se representa en el eje de las abscisas (X) y el tiempo transcurrido en el eje de las ordenadas (Y).
- La aplicación recibe los valores crudos en texto desde el driver y se encarga de realizar la corrección de escala y el casteo a valores numéricos, liberando al kernel de responsabilidades de formato visual.

La interfaz principal mantiene el control de la ventana, los botones y el ciclo de refresco del gráfico. Cuando el usuario cambia de GPIO, la app limpia las muestras anteriores, reinicia el eje temporal y actualiza el título del gráfico para indicar claramente la nueva señal que se está sensando, representando solo la lectura activa. Ese mismo flujo puede verse en la consola SSH de la Raspberry Pi virtual mientras la app queda ejecutándose. [Espacio reservado para la captura: consola SSH con la app ejecutándose]

La selección del GPIO también queda visible en la consola cuando la app escribe `0` o `1` sobre `/dev/sensor_driver` para cambiar de entrada. [Espacio reservado para la captura: consola SSH seteando GPIO]

### Relación con el driver

La app no conoce detalles internos del kernel ni del hardware GPIO. Su única dependencia es el contrato del archivo de dispositivo:

- escribir `0` o `1` para seleccionar la entrada,
- leer una cadena con el valor actual del GPIO,
- y repetir el proceso cada segundo.

Eso hace que la app sea independiente del mecanismo interno usado por el driver para llegar al hardware.

---

## Prueba conjunta

Para finalizar con el apartado práctico, se realizó una prueba utilizando la generación de señales, el CDD y la aplicación de usuario en conjunto para comprobar el correcto funcionamiento del sistema.

### Terminal 1 — Generador de señales
```bash
python3 signals_for_rpi_v5.py
```

### Terminal 2 — QEMU
```bash
./run.sh
```

### Terminal 3 — SSH dentro de la Raspberry Pi simulada
```bash
ssh pi@localhost -p 50022
sudo make load
python3 app.py
```
Finalmente, la generación de señales se puede ver a continuación, donde se muesta la señal cuadrada y la senoidal por separado.

<img width="1001" height="732" alt="Captura desde 2026-05-29 22-10-55" src="https://github.com/user-attachments/assets/15304adb-a1b1-4bb7-9080-5a43b4055db6" />

<img width="997" height="737" alt="Captura desde 2026-05-30 01-41-49" src="https://github.com/user-attachments/assets/aeee7c41-727e-4c61-a551-2178beec33b1" />

---

## Conclusiones

El trabajo permitió integrar espacio de usuario y espacio de kernel a través de un `character device` real, con una separación clara de responsabilidades. El driver resuelve el acceso al hardware GPIO desde el kernel, mientras que la aplicación se ocupa únicamente de la visualización y de la interacción con el usuario.

Una de las conclusiones principales es que el flujo correcto no consiste en leer GPIOs directamente desde Python, sino en delegar esa tarea al módulo del kernel y tratar al dispositivo como un archivo. Ese modelo simplifica la aplicación, mejora el aislamiento y aprovecha la infraestructura estándar de Linux para manejo de dispositivos.

También se comprobó que la API moderna de GPIO basada en descriptores es más adecuada que enfoques antiguos basados en sysfs. El driver resultante queda más cercano a las prácticas actuales del kernel y más fácil de mantener.

Por último, la solución final demuestra la separación entre:

- el hardware o su emulación,
- el driver que abstrae el acceso,
- y la aplicación de usuario que consume los datos y los presenta de forma gráfica.

---

## Referencias

- [Linux Kernel API Documentation](https://www.kernel.org/doc/htmldocs/kernel-api/index.html)
- [Playing with Linux Drivers](https://sysplay.github.io/books/LinuxDrivers/book/)
- [Repositorio de la cátedra](https://gitlab.com/sistemas-de-computacion-unc/device-drivers/)
- [Mapa del kernel de Linux](https://makelinux.github.io/kernel/map/)

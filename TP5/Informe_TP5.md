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

### Detalles de implementación

La app usa un hilo secundario para evitar bloquear la interfaz gráfica. Ese hilo:

- espera a que haya un GPIO seleccionado,
- abre el dispositivo en modo lectura,
- obtiene una línea de texto,
- la interpreta con un patrón del tipo `GPIO<numero>:<valor>`,
- y almacena la muestra en memoria para graficarla.

La interfaz principal mantiene el control de la ventana, los botones y el ciclo de refresco del gráfico. Cuando el usuario cambia de GPIO, la app limpia las muestras anteriores y reinicia el eje temporal para que la gráfica represente solo la señal activa.

### Relación con el driver

La app no conoce detalles internos del kernel ni del hardware GPIO. Su única dependencia es el contrato del archivo de dispositivo:

- escribir `0` o `1` para seleccionar la entrada,
- leer una cadena con el valor actual del GPIO,
- y repetir el proceso cada segundo.

Eso hace que la app sea independiente del mecanismo interno usado por el driver para llegar al hardware.

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

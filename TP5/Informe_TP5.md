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

Este trabajo práctico aborda la programación de módulos del kernel de Linux, con foco en la construcción de un **Character Device Driver (CDD)**. El objetivo final es desarrollar un driver que permita sensar dos señales externas con un período de un segundo, y una aplicación de usuario que lea una de las señales y la grafique en tiempo real.

El desarrollo se realizó sobre una PC con Ubuntu 24.04 y kernel 6.17, simulando las señales por software en lugar de hardware real (Raspberry Pi).

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

---

## Aplicación de usuario

---

## Conclusiones

---

## Referencias

- [Linux Kernel API Documentation](https://www.kernel.org/doc/htmldocs/kernel-api/index.html)
- [Playing with Linux Drivers](https://sysplay.github.io/books/LinuxDrivers/book/)
- [Repositorio de la cátedra](https://gitlab.com/sistemas-de-computacion-unc/device-drivers/)
- [Mapa del kernel de Linux](https://makelinux.github.io/kernel/map/)

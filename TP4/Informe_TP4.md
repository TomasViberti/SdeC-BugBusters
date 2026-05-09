---

<div align="center">

# Universidad Nacional de Córdoba
## Facultad de Ciencias Exactas, Físicas y Naturales

---

### Sistemas de Computación
# Trabajo Práctico N°4
## Módulos de kernel y llamadas a sistema

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

## Introducción

El kernel de Linux tiene un diseño modular que permite cargar y descargar fragmentos de código en tiempo de ejecución sin necesidad de reiniciar el sistema. 
Estos fragmentos se denominan módulos del kernel y permiten extender sus funcionalidades dinámicamente, siendo los drivers de dispositivos el caso de uso más común.
En este trabajo práctico se explora el desarrollo, compilación, carga y descarga de módulos de kernel, así como los conceptos de espacio de usuario vs espacio de kernel, 
el sistema de archivos `/proc`, la API de acceso a archivos de Linux y la seguridad en la carga de módulos mediante firma digital.

## Objetivo general

Comprender el funcionamiento interno del kernel de Linux a través del desarrollo y análisis de módulos, entendiendo las diferencias entre un programa de usuario y un módulo de kernel, 
los mecanismos de seguridad disponibles y la interacción entre el hardware y el sistema operativo mediante drivers.

---

## Preparación del entorno

```bash
sudo apt-get update
sudo apt-get install build-essential checkinstall kernel-package linux-source
sudo apt-get install linux-headers-$(uname -r)
```

Verificar la correcta instalación de los headers del kernel:

```bash
ls /lib/modules/$(uname -r)/build
```

El código fuente del TP se integró como submódulo de Git dentro del repositorio del grupo, en la carpeta `TP4/`:

```bash
git submodule add https://gitlab.com/sistemas-de-computacion-unc/kenel-modules.git TP4/kenel-modules
```

Esto permite mantener el código del TP vinculado al repositorio original sin duplicarlo.

---

## Compilar el primer módulo

```bash
cd TP4/kenel-modules/part1
ls
cat mimodulo.c
make
```

<img width="1408" height="600" alt="image" src="https://github.com/user-attachments/assets/260516a7-97fe-4268-bc36-46cd89702e8a" />

Generó el archivo `mimodulo.ko`.

```bash
ls *.ko
```

Ahora cargamos el módulo al kernel:

```bash
sudo insmod mimodulo.ko
```

Y verificamos que se cargó:

```bash
sudo dmesg | tail
```

<img width="1177" height="331" alt="image" src="https://github.com/user-attachments/assets/4b5b6647-9b52-41cf-a9c4-a372b516c15b" />

Ahora verificamos que aparece en la lista de módulos cargados:

```bash
lsmod | grep mimodulo
```

Y luego lo descargamos:

```bash
sudo rmmod mimodulo
sudo dmesg | tail
```

<img width="1195" height="326" alt="image" src="https://github.com/user-attachments/assets/a507fe80-f6c2-486e-a5e1-00363f2e4f38" />


Ahora ejecutamos:

```bash
modinfo mimodulo.ko
```

Y también este para comparar con un módulo oficial del kernel:

```bash
modinfo /lib/modules/$(uname -r)/kernel/crypto/des_generic.ko
modinfo /lib/modules/$(uname -r)/kernel/crypto/des_generic.ko.zst
```

<img width="1167" height="274" alt="image" src="https://github.com/user-attachments/assets/e1f66445-5f77-422e-9882-a42768072225" />

<img width="1561" height="971" alt="image" src="https://github.com/user-attachments/assets/e672bc02-651f-4a10-9609-b4306e6d7c88" />

---
## Desafio 1

Para evitar que el kernel cargue código malicioso, se utiliza la verificación de firmas. Si un módulo no está firmado por una clave de confianza, el kernel lo rechaza.

¿Cómo evitar módulos no firmados?

Para forzar esta seguridad, se debe configurar el kernel con el parámetro module.sig_enforce=1. Esto hace que el kernel solo acepte módulos cuya firma sea válida, bloqueando cualquier intento de inyectar código no autorizado (común en rootkits).

### Otras medidas de seguridad del Kernel:

KSPP (Kernel Self Protection Project): Conjunto de parches para mitigar exploits.

KASLR (Kernel Address Space Layout Randomization): Aleatoriza dónde se carga el kernel en memoria para que los atacantes no sepan dónde "golpear".

Desactivar sysrq: Evita que alguien con acceso físico use combinaciones de teclas para colapsar o reiniciar el sistema.

Restringir dmesg: Evitar que usuarios no privilegiados vean logs del kernel (donde a veces se filtran direcciones de memoria).

### Checkinstall: Empaquetando un "Hello Word"

Checkinstall es una herramienta que reemplaza el clásico sudo make install. En lugar de esparcir archivos por todo el sistema sin control, genera un paquete (.deb, .rpm o Slackware) y lo instala usando el gestor de paquetes oficial. Permite desinstalar o actualizar software compilado desde el código fuente de forma limpia y organizada.

Primero se instala checkinstall:
<img width="1011" height="30" alt="image" src="https://github.com/user-attachments/assets/a1ecc99d-56e4-4cad-8f4f-a998d04d90e3" />

Luego compilamos el hello world:
<img width="1013" height="71" alt="image" src="https://github.com/user-attachments/assets/818685d8-f2ff-455d-8260-40b9077dabe7" />

Al compilar se genera el ejecutable ```hello```. Al haber compilado con un makefile que contiene lo siguiente:
```
# Nombre del ejecutable
TARGET = hello

# Regla por defecto: compilar
all:
	gcc hello.c -o $(TARGET)

# Regla de instalación (la que usará checkinstall)
install:
	install -m 0755 $(TARGET) /usr/local/bin/

# Regla para limpiar archivos temporales
clean:
	rm -f $(TARGET)
```
Y luego ejecutar el comando sudo checkinstall, el programa hello queda empaquetado en el kernel, perteneciendo al sistema operativo. Esto lo podemos chequear si ejecutamos el comando ```hello``` desde una terminal cualquiera y veremos que se ejecuta como si se tratara de otro comando:

<img width="804" height="140" alt="image" src="https://github.com/user-attachments/assets/38cc3c7a-f31a-4430-bd92-046855fa2e00" />

### Hardware Info (hwinfo)

En primera instancia debemos instalar la herramienta ```hwinfo```:

Ejecutamos el comando ```sudo apt install hwinfo```.

Luego corremos el comando ```hwinfo --short```:


---
## Desafio 2
---

## Preguntas del informe

### 1. ¿Qué diferencias se pueden observar entre los dos modinfo? 

Las diferencias clave son:

| Campo | `mimodulo.ko` | `des_generic.ko.zst` |
|---|---|---|
| **alias** | ninguno | múltiples (des, des3_ede, etc.) |
| **intree** | no aparece | `Y` (es oficial del kernel) |
| **sig_id** | no tiene | `PKCS#7` |
| **signer** | no tiene | `Build time autogenerated kernel key` |
| **sig_hashalgo** | no tiene | `sha512` |
| **signature** | no tiene | firma completa |
| **formato** | `.ko` | `.ko.zst` (comprimido) |

La diferencia más importante: **`des_generic` está firmado y es `intree` (oficial), `mimodulo` no tiene firma ni es oficial**. Por eso al cargar `mimodulo` apareció el warning `module verification failed`.

### 2. ¿Qué divers/modulos estan cargados en sus propias pc?

#### Comando utilizado

```bash
lsmod
```

La salida se guardó en un archivo para comparar con los compañeros:

```bash
lsmod > TP4/lsmod_gcapdevila.txt
```

### Salida de lsmod (Gastón - VirtualBox)

```
Module                  Size  Used by
isofs                  61440  1
snd_seq_dummy          12288  0
snd_hrtimer            12288  1
vboxsf                 45056  0
vboxguest              57344  7 vboxsf
vboxvideo              36864  0
drm_vram_helper        24576  1 vboxvideo
xt_conntrack           12288  1
xt_MASQUERADE          16384  1
bridge                425984  0
stp                    12288  1 bridge
llc                    16384  2 bridge,stp
xfrm_user              65536  1
xfrm_algo              16384  1 xfrm_user
xt_set                 20480  0
ip_set                 61440  1 xt_set
nft_chain_nat          12288  5
nf_nat                 61440  2 nft_chain_nat,xt_MASQUERADE
nf_conntrack          192512  3 xt_conntrack,nf_nat,xt_MASQUERADE
nf_defrag_ipv6         24576  1 nf_conntrack
nf_defrag_ipv4         12288  1 nf_conntrack
xt_addrtype            12288  4
nft_compat             20480  6
nf_tables             385024  57 nft_compat,nft_chain_nat
overlay               221184  0
qrtr                   53248  2
snd_intel8x0           53248  1
snd_ac97_codec        196608  1 snd_intel8x0
ac97_bus               12288  1 snd_ac97_codec
intel_rapl_msr         20480  0
joydev                 32768  0
snd_pcm               200704  2 snd_intel8x0,snd_ac97_codec
snd_seq_midi           24576  0
snd_seq_midi_event     16384  1 snd_seq_midi
snd_rawmidi            57344  1 snd_seq_midi
snd_seq               122880  9 snd_seq_midi,snd_seq_midi_event,snd_seq_dummy
intel_rapl_common      53248  1 intel_rapl_msr
snd_seq_device         16384  3 snd_seq,snd_seq_midi,snd_rawmidi
snd_timer              53248  3 snd_seq,snd_hrtimer,snd_pcm
snd                   143360  11 snd_seq,snd_seq_device,snd_intel8x0,snd_timer,snd_ac97_codec,snd_pcm,snd_rawmidi
soundcore              16384  1 snd
ghash_clmulni_intel    12288  0
aesni_intel            98304  0
vmwgfx                471040  3
input_leds             12288  0
drm_ttm_helper         16384  4 vmwgfx,drm_vram_helper,vboxvideo
i2c_piix4              32768  0
mac_hid                12288  0
ttm                   126976  3 vmwgfx,drm_vram_helper,drm_ttm_helper
i2c_smbus              20480  1 i2c_piix4
serio_raw              20480  0
binfmt_misc            24576  1
sch_fq_codel           24576  2
msr                    12288  0
parport_pc             53248  0
ppdev                  24576  0
lp                     32768  0
parport                73728  3 parport_pc,lp,ppdev
efi_pstore             12288  0
nfnetlink              20480  5 nft_compat,nf_tables,ip_set
dmi_sysfs              20480  0
ip_tables              32768  0
x_tables               65536  6 xt_conntrack,nft_compat,xt_addrtype,xt_set,ip_tables,xt_MASQUERADE
autofs4                57344  2
hid_generic            12288  0
vga16fb                32768  0
usbhid                 77824  0
pata_acpi              12288  0
hid                   262144  2 usbhid,hid_generic
vgastate               20480  1 vga16fb
psmouse               217088  0
video                  77824  0
wmi                    28672  1 video
ahci                   49152  1
e1000                 180224  0
libahci                53248  1 ahci
```

### Módulos destacados

| Módulo | Descripción |
|---|---|
| `vboxsf`, `vboxguest`, `vboxvideo` | VirtualBox Guest Additions — confirman que el sistema corre en VirtualBox |
| `snd_intel8x0`, `snd_ac97_codec` | Driver de audio virtualizado |
| `e1000` | Driver de red Intel (virtualizado) |
| `ahci` | Controlador de disco SATA |
| `aesni_intel` | Aceleración de cifrado AES por hardware |
| `nf_tables`, `nf_nat` | Firewall y NAT del kernel |

Cada integrante guarda su salida y se compara con `diff`:

```bash
lsmod > TP4/lsmod_<inicial_nombre+apellido>.txt
diff TP4/lsmod_gcapdevila.txt TP4/lsmod_<compañero>.txt
```

> **Nota:** La comparación completa y el diff entre integrantes se agregará una vez que todos los miembros del grupo ejecuten el comando en sus propias máquinas.

### 3. ¿Cuáles módulos no están cargados pero están disponibles?

#### Módulos disponibles en el sistema

Los módulos disponibles se encuentran en `/lib/modules/$(uname -r)/kernel/`. Se pueden ver las categorías principales:

```bash
ls /lib/modules/$(uname -r)/kernel/
```

Para contar el total de módulos disponibles:

```bash
find /lib/modules/$(uname -r)/kernel/ -name "*.ko*" | wc -l
```

#### Módulos cargados actualmente

```bash
lsmod | wc -l
```

<img width="1272" height="171" alt="image" src="https://github.com/user-attachments/assets/a438681a-4014-44bb-a862-b656e468642d" />

Esto ilustra el diseño modular del kernel: solo carga en memoria lo estrictamente necesario para el hardware y funcionalidades en uso.

### ¿Qué pasa cuando el driver de un dispositivo no está disponible?

Hay tres escenarios posibles:

- **El módulo existe pero no está cargado** → el kernel puede cargarlo automáticamente al detectar el dispositivo mediante `modprobe`. El dispositivo funciona normalmente.
- **El módulo no existe para ese hardware** → el dispositivo no funciona. No aparece en `/dev` y el sistema no lo reconoce. Un ejemplo típico es conectar un adaptador WiFi sin driver disponible: no aparece ninguna interfaz de red.
- **El módulo existe pero falla al cargar** → puede causar un kernel panic o simplemente rechazar la carga, dejando el dispositivo inutilizable.

### 4. Correr hwinfo en una pc real con hw real y agregar la url de la información de hw en el reporte. 

Luego de ejecutar el comando ```hwinfo``` con las flags para obtener el link, vemos lo siguiente:
<img width="1182" height="45" alt="image" src="https://github.com/user-attachments/assets/bf2b4e74-b2cf-4bde-b7ff-0926ba790537" />

El link propiamente dicho es: https://termbin.com/f7jg7


### 8. Firmado de un módulo de Kernel

<img width="1855" height="352" alt="image" src="https://github.com/user-attachments/assets/7048f971-7b91-4e39-9ce7-1ce89db26210" />

<img width="1119" height="93" alt="image" src="https://github.com/user-attachments/assets/a62a8e49-b9ba-405b-9cef-1f81fe7679ef" />


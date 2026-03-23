# UNIVERSIDAD NACIONAL DE CÓRDOBA
# FACULTAD DE CIENCIAS EXACTAS, FÍSICAS Y NATURALES

## SISTEMAS DE COMPUTACIÓN	
## Trabajo Práctico N°1: Rendimiento
### Grupo: BugBusters

- Alfici Facundo
- Capdevila Gastón
- Viberti Tomas

### Docentes
- Jorge, Javier Alejandro
- Solinas, Miguel

### 2026

---
# Informé PC de Gastón Capdevila

## GPROF

Debemos asegurarnos de que la generación de perfiles esté habilitada cuando se complete la compilación del código. Esto es posible al agregar la opción '-pg' en el paso de compilación.

<img width="1198" height="220" alt="image" src="https://github.com/user-attachments/assets/1911b2b5-1a7b-48f8-a526-2d20faa640bd" />

**Ejecutar la herramienta gprof**

<img width="1062" height="91" alt="image" src="https://github.com/user-attachments/assets/8c723847-1411-449d-b8f0-d9c99792127c" />

<img width="755" height="919" alt="image" src="https://github.com/user-attachments/assets/ed721d31-ae8a-4713-87da-099e9765bcbe" />

---

### 1. Suprima la impresión de funciones declaradas estáticamente (privadas) usando -a
Aplicando la flag -a se suprimen las funciones declaradas estáticamente (privadas). Por ejemplo, en este caso la func2 no devolverá información por ser estática.

**gprof -a test_gprof gmon.out > analysis.txt**

<img width="878" height="928" alt="image" src="https://github.com/user-attachments/assets/74e020b4-22b1-4cff-bf57-dbb46eda84d5" />

### 2. Elimine los textos detallados usando -b
Aplicando la flag -b se suprimen los textos detallados. Entonces podemos observar que se obtienen solo gráficos y resultados concretos del test.

**gprof -b test_gprof gmon.out > analysis.txt**

<img width="836" height="896" alt="image" src="https://github.com/user-attachments/assets/ddc26439-77a6-4a0f-8dd8-a7e688308c91" />

### 3. Imprima solo perfil plano usando -p
Aplicando la flag -p se obtiene como salida solo el perfil plano.

**gprof -p test_gprof gmon.out > analysis.txt**

<img width="820" height="929" alt="image" src="https://github.com/user-attachments/assets/bc2c0fa4-dbe5-42db-8c72-4b9a49b593a3" />

### 4. Imprimir información relacionada con funciones específicas en perfil plano

**gprof -pfunc1 -b test_gprof gmon.out > analysis.txt**

<img width="721" height="203" alt="image" src="https://github.com/user-attachments/assets/dad0e1ce-a3fb-43c6-8c97-a904e99525ec" />

### Grafico

<img width="1376" height="240" alt="image" src="https://github.com/user-attachments/assets/871abba5-a033-4e70-8e04-f34d7b19d195" />

<img width="233" height="396" alt="image" src="https://github.com/user-attachments/assets/b404db71-2cf2-4a8b-8ca5-f89b5d9fb53d" />

---

## Profiling con linux perf

<img width="1075" height="554" alt="image" src="https://github.com/user-attachments/assets/bb7e52c2-c003-4693-94a5-97d17d0da075" />

**$ sudo perf report**

<img width="937" height="957" alt="image" src="https://github.com/user-attachments/assets/5d1f0dea-d879-4554-b346-7039850d642c" />

- La imagen muestra el reporte de perf con las funciones de test_gprof ordenadas por tiempo de CPU. 
- Las funciones propias del programa son `func1` (49,10%), `func2` (40,51%), `new_func1` (3,95%) y `main` (2,68%). 
- El resto son funciones del kernel Linux (`[k]`) que se pueden ignorar.

---
# Informe PC Tomás Viberti

## GPROF

![alt text](image-15.png)

![alt text](image-16.png)

A continuación se muestra el analysis.txt obtenido:

![alt text](image-17.png)

![alt text](image-18.png)

Si se ejecuta el comando con el comando de supresión de las funciones declaradas estáticamente, se obtiene lo siguiente:

![alt text](image-19.png)

![alt text](image-20.png)

Si se eliminan los textos detallados usando el comando -b:

![alt text](image-21.png)

![alt text](image-22.png)

Si ahora solo se imprime el perfil plano con el comando -p:

![alt text](image-23.png)

![alt text](image-9.png)

Y si se quiere imprimir información especifica de una función en perfil plano, en este caso func1, se obtiene:

![alt text](image-24.png)

![alt text](image-11.png)

Por otro lado, si se genera el gráfico con gprof2dot mediante el comando “gprof ./test_gprof gmon.out | gprof2dot | dot -Tpng -o grafo_perfilado.png”, se obtiene el siguiente gráfico:

![alt text](image-12.png)

Por último si se ejecuta el codigo con perfilado usando perf:

![alt text](image-25.png)

![alt text](image-26.png)

# Informe de Facundo ALFICI

## GPROF

Como primera medida, se compiló el código habilitando la creación de perfiles.

![alt text](Cap1.png)

Esto generó un archivo binario, el cual se ejecutará para generar la información de perfiles.

![alt text](Cap9.png)

Con esto, se generó el archivo analysis utilizando la herramienta GPROF.

![alt text](Cap10.png)

![alt text](Cap2.png)

Empezando la customización de las flags de salida de la herramienta, se plantea suprimir la impresión de funciones estáticas, utilizando la siguiente línea de código.
**$ gprof -a test_gprof gmon.out > analysis_3.txt**

![alt text](Cap4.png)

Luego, se eliminarán los textos detallados con -b
**$ gprof -b test_gprof gmon.out > analysis_3.txt**

![alt text](Cap5.png)

Posteriormente, se imprimirá unicamente el perfil plano utilizando -p
**$ gprof -p -b test_gprof gmon.out > analysis_3.txt**

![alt text](Cap6.png)

Y, para terminar esta etapa, se imprimirá solamente la información de la función específica "func1"
**$ gprof -pfunc1 -b test_gprof gmon.out > analysis_3.txt**

![alt text](Cap7.png)

Por otro lado, se quiere graficar utilizando dos herramientas diferentes.
Primeramente, se usará gprof2dot para generar una visualización de la salida de gprof.

![alt text](grafico_perf.png)

Para la segunda herramienta, se utilizará Linux perf.

![alt text](Cap12.png)

![alt text](Cap8.png)

## Benchmarks
# Reporte de Benchmarking y Rendimiento del Sistema

 Un benchmark es una prueba de rendimiento estandarizada que se realiza sobre un componente o un sistema de software para medir su capacidad, rendimiento, eficacia. Los benchmarks realizan una serie de tareas predefinidas y exigentes, y mide cuánto tiempo tarda nuestro sistema en completarlas. En base al tiempo de ejecución que conlleve, se obtiene un score.

 Hay una gran cantidad de benchmarks disponibles los cuales se ajustan a cada componente o sistema operativo, como windows o Linux. Como ejemplo se pueden mencionar:

### **Linux**
* **Llama.cpp**: Rendimiento en modelos de lenguaje (IA).
* **Timed Linux Kernel Compilation**: Velocidad de procesamiento de archivos.
* **Blender**: Renderizado 3D.
* **Hashcat**: Criptografía y seguridad.
* **OpenVINO GenAI / OpenGL**: Gráficos y aceleración de IA.

### **Windows**
* **PCMark 10**: Rendimiento general de oficina.
* **UserBenchmark**: Comparativa rápida de componentes.
* **Cinebench**: Renderizado de CPU puro.
* **Geekbench**: Rendimiento sintético mono y multihilo.
* **3DMark (Time Spy / Port Royal)**: Rendimiento gráfico en gaming.


## Tareas Diarias y Benchmarks Asociados

Esta tabla resume la relación entre el flujo de trabajo personal y la herramienta de medición ideal:

| Tarea Diaria | Benchmark | Detalle del Benchmark |
| :--- | :--- | :--- |
| **Gaming (AAA)** | 3DMark Time Spy / Port Royal | Esta tarea implica mucho poder de procesamiento tanto por parte del CPU como de la GPU. Este benchmark permite estresar dichos componentes a través de tareas de renderizado pesado. |
| **Programación** | Geekbench 6 (Multi-core) | En esta tarea se incluyen actividades de programacion simples hasta actividades de programación multihilos, por ejemplo para levantar microservicios en docker. Este benchmark mide la capacidad de procesar hilos en paralelo y recae sobre el CPU. |
| **Compilación de código** | Linux Kernel Compilation | Para la realización de varios proyectos académicos utilizo Linux. Este bench mide el tiempo que tarda el CPU en procesar muchos archivos pequeños.|
| **Pestañas multitarea** | PCMark 10 | Este bench simula la navegación web, edición de documentos y videollamadas de manera simultánea.|


## Análisis de Rendimiento (Casos Prácticos)

Utilizando los datos de la tarea **Timed Linux Kernel Compilation**, realizamos las siguientes comparativas:

### 1. Comparativa de Rendimiento
* **Intel Core i5-13600K**: $72 \pm 5$ seg.
* **AMD Ryzen 9 5900X**: $76 \pm 8$ seg. 

A simple vista, podemos decir que el i5 13600K tiene mejor rendimiento para compilar el Kernel de Linux. Si hacemos la comparativa utilizando la fórmula:

$$\frac{\text{Rendimiento } i5}{\text{Rendimiento } R9} = \frac{76s}{72s} = 1.055$$

> **Resultado:** El **Intel Core i5-13600K** es un **5,5% más rápido** que el Ryzen 9 5900X en esta tarea.

### 2. Cálculo de Aceleración ($S$)
Si utilizamos un RYzen 9 7950X 16 core, para el cual el tiempo es de 50 +/- 6 seg, entonces la aceleracion es:

$$S = \frac{T_{viejo}}{T_{nuevo}} = \frac{76s}{50s} = 1.52$$

> **Resultado:** El Ryzen 9 7950X (16 core) ofrece una aceleración de **1.52x**, lo que significa que es un **52% más rápido** que el modelo 5900X 12 core.

# Tiempo de programa según variación de frecuencia

Se desea ejecutar sobre una ESP32 un código que demore alrededor de 10 segundos para una frecuencia determinada de reloj. El objetivo es variar la frecuencia de reloj y permitir ver las diferencias obtenidas en el tiempo del programa.

En primera medida, se presentará el código del programa:
```c
void setup() {
  Serial.begin(115200);
  // Configurar la frecuencia
  setCpuFrequencyMhz(80);
  Serial.print("Frecuencia de CPU configurada: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");

  unsigned long inicio = millis();

  // Bucle
  volatile unsigned long resultado = 0;
  for (unsigned long i = 1; i <= 53000000; i++) {
    resultado += i ^ (i % 123);
  }
  unsigned long fin = millis();
  unsigned long tiempo = fin - inicio;
  Serial.print("Resultado final: ");
  Serial.println(resultado);
  Serial.print("Tiempo de ejecución: ");
  Serial.print(tiempo / 1000.0);
  Serial.println(" segundos");
}
void loop() {}
```
Con este código, se puede obtener la siguiente respuesta para una frecuencia de 80MHz.

![alt text](Cap_esp32_80MHz.png)

Mientras que, para los 160MHz, se tiene que:

![alt text](Cap_esp32_160MHz.png)

Concluyendo así que, para un cambio de frecuencia, se nota una diferencia clara entre los tiempos de programa, donde el mismo se ve reducido a la mitad para una frecuencia que es el doble de la original.

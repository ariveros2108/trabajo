
**Institución:** Universidad Tecnológica Metropolitana (UTEM)
**Autores:** Alexander Riveros, Sebastián Insulza, Sebastián Antileo
**Sección:** 411

## Descripción
Este proyecto implementa un sistema de ingesta y análisis de datos de alto rendimiento desarrollado en C++. El software descarga, parsea y procesa de forma paralela un lote masivo de archivos transaccionales (CSV) alojados en un servidor SFTP, correlacionando identificadores de clientes con una API REST externa para calcular métricas financieras basadas en género.

El sistema fue diseñado con una arquitectura tolerante a fallos, incorporando caché en memoria RAM, paralelismo simétrico mediante OpenMP y persistencia de sesiones HTTP para evitar saturación de red.

## Tecnologías y Dependencias
El proyecto está diseñado para entornos Linux (Ubuntu/Debian) y requiere las siguientes librerías para su compilación:

* **Compilador:** GCC compatible con C++17 (`g++`)
* **Gestión de compilación:** CMake y Make
* **Paralelismo:** OpenMP (`libomp-dev`)
* **Redes y API:** libcurl (`libcurl4-openssl-dev`)
* **SFTP:** libssh2 (`libssh2-1-dev`)

Puedes instalar todas las dependencias ejecutando:

sudo apt update
sudo apt install build-essential cmake libomp-dev libcurl4-openssl-dev libssh2-1-dev


Instrucciones de Compilación
El proyecto utiliza CMake para facilitar la configuración independiente de la plataforma. Para compilar el código fuente, sitúate en la raíz del proyecto y ejecuta los siguientes comandos:


# 1. Crear directorio de construcción
mkdir build
cd build

# 2. Generar archivos Make
cmake ..

# 3. Compilar utilizando 4 hilos para mayor velocidad
make -j4

Ejecución del Sistema (Dos Terminales)
Dado que el sistema cuenta con un entorno de simulación local y un modelo de ejecución paralela asíncrona, se recomienda el uso de dos terminales simultáneas para su correcta ejecución y monitoreo.

Opción A: Modo Local (Pruebas)
Este modo aísla la latencia externa ejecutando las peticiones contra el servidor mock local.

Terminal 1: Levantar la API Simulada


# Desde la raíz del proyecto, inicializa el servidor local en el puerto 8080
python3 mock_api.py
(Mantén esta terminal abierta).

Terminal 2: Ejecutar el Proyecto


# En una nueva pestaña, navega al directorio de construcción
cd build
./proyecto

# Cuando el prompt lo solicite, ingresa: 2
Opción B: Modo Producción (Conexión API)


Terminal 1: Ejecutar el Proyecto


cd build
./proyecto

 
Si deseas ver cómo el sistema gestiona fallos de la API o latencias, inspecciona el log:


tail -f log.txt

Salida y Resultados
Al finalizar, el programa imprimirá el consolidado en pantalla y generará automáticamente dos archivos en el directorio de ejecución:

resultados.txt: Contiene la sumatoria total por género y el tiempo de ejecución.

log.txt: Archivo de auditoría (registra errores de API, parseo o de red si los hubiese).
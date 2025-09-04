# Sistemas-Operativos-1
--------------------------------------------------------------------------------------------------------------------------------------
Buenos Aires, 22 de Agosto

Estimados ingenieros,
Por este medio, en mi carácter de Presidente de la Nación y coordinador del Programa Nacional de Monitoreo Sanitario, me dirijo a ustedes para encargar la primera etapa de desarrollo del nodo local. Esta etapa es crítica para establecer la línea base de funcionamiento del sistema y garantizar que toda decisión futura se apoye en información confiable.
El Ministerio de Salud nos ha indicado que antes de pensar en diagnósticos, algoritmos de análisis o comunicación con centros de control, debemos asegurarnos de que el nodo sea capaz de observar y registrar su propio estado de forma constante. No podemos permitir que un equipo falle sin que nadie lo note, ni que se desconozca en qué condiciones opera.
En esta primera entrega, necesito que el software que ustedes diseñen sea capaz de, al menos:
Acceder y obtener datos reales sobre el estado del sistema operativo que ejecuta el nodo. Esto incluye, como mínimo, información sobre uso de CPU, disponibilidad de memoria y carga general del sistema.


Registrar esas métricas de forma estructurada para que puedan ser interpretadas por técnicos o sistemas de análisis externos.


Mantener un registro histórico suficiente para identificar cambios y tendencias, con el debido cuidado para no saturar el almacenamiento local.


Contar con un mecanismo que permita la revisión inmediata del estado actual, sin depender de sistemas externos.


No me interesa, en este momento, la estética ni la sofisticación de la interfaz. Sí me importa que la información sea correcta, precisa y confiable. Estos datos deberán estar disponibles para que, en etapas futuras, podamos integrarlos en tableros de control como los que utilizan en el sector industrial y hospitalario.
Recuerden que estamos en una etapa inicial: lo que entreguen ahora servirá como base para las siguientes funciones. Si en esta instancia fallamos en la exactitud o en la consistencia de la información, todo lo que construyamos después se verá comprometido.
Confío en su criterio para organizar el trabajo y elegir las herramientas del sistema que les permitan cumplir con estos objetivos. Espero su entrega en la fecha acordada y que, junto con el software, presenten un breve informe que documente cómo se obtiene la información y cómo se garantiza su integridad.
Atentamente,

Juan Salvo
 Presidente de la Nación Argentina
 Coordinador – Programa Nacional de Monitoreo Sanitario

 ---------------------------------------------------------------------------------------------------------------------------------------

Monitoreo básico y observabilidad – Detalles resumidos,
Propósito,
,
El objetivo de este primer trabajo es implementar un módulo de observabilidad para el nodo local de monitoreo sanitario.
El módulo deberá capturar, procesar y registrar métricas clave del sistema operativo en ejecución, garantizando que la información sea precisa, estructurada y accesible para su consulta local y futura integración con herramientas de visualización como Grafana. (editado)
[19:35]
Alcance funcional,
,
El sistema debe:

Obtener métricas en tiempo real desde el pseudo-sistema de archivos /proc.,
Registrar la información en formato JSON en un directorio predefinido: /var/lib/monitoreo/.,
Capturar métricas en intervalos regulares (mínimo cada 5 segundos, configurable).,
Permitir la consulta inmediata de métricas actuales mediante Grafana u otra herramienta de observabilidad que lea directamente los registros.,
Evitar dependencias externas complejas: el desarrollo debe realizarse con CMake y Conan, utilizando funciones estándar y llamadas al sistema.,
 (editado)
[19:36]
Observabilidad en el contexto del proyecto,
,
La observabilidad es la capacidad de un sistema para exponer información relevante sobre su estado interno a partir de sus salidas externas.

En este caso, el nodo debe describir:
Qué recursos utiliza (CPU, memoria, disco).,
En qué condiciones está operando (carga, procesos activos).,
Cómo evoluciona su estado a lo largo del tiempo.,

La observabilidad permite:
Detectar problemas antes de que se conviertan en fallas críticas.,
Analizar el rendimiento y la capacidad del sistema.,
Proporcionar datos confiables para la toma de decisiones de mantenimiento o ajuste.,
[19:36]
Directorios y archivos relevantes,
,
CPU: /proc/stat → estadísticas generales de CPU (usuario, sistema, idle, etc.).,
Memoria: /proc/meminfo → memoria física total, memoria libre, buffers y cachés.,
Carga del sistema: /proc/loadavg → carga media en intervalos de 1, 5 y 15 minutos.,
Procesos: /proc/[pid]/status → información detallada de un proceso en particular (opcional para futuras extensiones).,

 El sistema no debe depender de utilitarios externos como top o free; se debe parsear directamente la información en /proc.
[19:36]
Métricas mínimas a capturar,
,
CPU
Tiempo total de CPU usado por usuario.,
Tiempo total de CPU usado por el sistema.,
Porcentaje de utilización de CPU.,
,

Memoria
Memoria total.,
Memoria libre.,
Memoria usada (total - libre - buffers - caché).,
,

Carga
Promedio de carga en 1, 5 y 15 minutos.,
,

Tiempo
Timestamp de la medición (formato UNIX epoch o ISO8601).,
,
[19:37]
Estructura de almacenamiento de métricas,
,
Directorio principal: /var/lib/monitoreo/,
Nombre sugerido del archivo: metrics-YYYYMMDD.log,
Formato: NDJSON (un objeto JSON por línea).,

Ejemplo:
{"timestamp":1693262400,"cpu_user":23.5,"cpu_system":12.1,"mem_total":8048580,"mem_free":1456292,"load_1m":0.42,"load_5m":0.35,"load_15m":0.30}
 (editado)
[19:37]
Criterios de aceptación,
,
Lectura directa de los archivos en /proc sin dependencias externas.,
Registro persistente y estructurado en formato NDJSON.,
Intervalo de muestreo mínimo de 5 segundos, configurable.,
Integración con Grafana mediante un dashboard básico que muestre las métricas.,
Código en CMake y Conan, compilable con gcc.,
Análisis estático con cppcheck,scan-build y valgrind sin errores críticos.,
QA aceptable según revisión docente.,

Pruebas de integración,
Se deberán verificar:

Lectura y parseo correcto de /proc.,
Creación del archivo de métricas.,
Formato de salida válido.,
Cobertura mínima de pruebas del 20 %, incluyendo al menos un caso límite (datos incompletos o formato inesperado).

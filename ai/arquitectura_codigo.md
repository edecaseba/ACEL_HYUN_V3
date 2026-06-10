<?xml version="1.0" encoding="UTF-8"?>
<code_architecture environment="VS Code + PlatformIO" framework="Arduino" language="C++20">
    <build_management>
        <strategy>Resolver configuraciones de toolchain, optimizaciones, macros de preprocesador y flags de compilación prioritariamente a través de platformio.ini y Advanced Scripting (Python).</strategy>
    </build_management>
    <memory_management policy="Zero Dynamic RAM Post-Init">
        <init_phase>Se permite asignación estática o configuraciones iniciales estrictamente controladas.</init_phase>
        <runtime_phase>Prohibido el uso de memoria dinámica en loops, interrupciones o tareas del RTOS.</runtime_phase>
        <forbidden_elements>
            <element>String (Clase nativa de Arduino)</element>
            <element>new</element>
            <element>delete</element>
            <element>malloc</element>
            <element>free</element>
            <element>realloc</element>
            <element>Contenedores STL con reasignación dinámica en runtime</element>
        </forbidden_elements>
        <mitigation>Uso mandatorio de almacenamiento estático (static), asignación en pila con tamaños fijos conocidos en tiempo de compilación y tipos seguros de ancho fijo (&lt;cstdint&gt;).</mitigation>
        <goal>Evitar fragmentación de memoria por completo y priorizar almacenamiento estático.</goal>
    </memory_management>
    <coding_standard base="MISRA C:2012 / MISRA C++:2008">
        <rule>Control estricto sobre casts de tipos (utilizar static_cast explícito, evitar casts implícitos peligrosos).</rule>
        <rule>Estructuras de control de flujo con llaves obligatorias.</rule>
        <rule>Prohibido código inalcanzable o variables sin inicializar.</rule>
        <rule>Funciones con un único punto de retorno o salidas tempranas limpias que no comprometan recursos.</rule>
    </coding_standard>
</code_architecture>
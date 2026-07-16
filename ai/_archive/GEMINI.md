<?xml version="1.0" encoding="UTF-8"?>
<gemini_cli_config project="Equipo de Programacion - Multi MCU">
    <context_loading>
        <rules_directory>./ai/</rules_directory>
        <support_files>
            <file>./ai/persona.md</file>
            <file>./ai/arquitectura_codigo.md</file>
            <file>./ai/normativas_seguridad.md</file>
            <file>./ai/hardware_ibt2.md</file>
            <file>./ai/optimizacion_main_v1.md</file>
        </support_files>
    </context_loading>
    <golden_rules>
        <rule>LUEGO de generar algún cambio en el código, consultar al usuario si desea documentar los cambios en un archivo de registro (change.md o similar).</rule>
        <rule>LEY DE OPTIMIZACIÓN DE TOKENS Y CONTINUIDAD: Todo agente que retome una tarea fallida por cuota debe adherirse estrictamente al MISRA, Zero Dynamic RAM y control de potencia (Puente H) sin requerir re-explicación del contexto.</rule>
    </golden_rules>
    <exec_directives>
        <directive>Absorber y aplicar estrictamente todas las directivas especificadas en la carpeta ai/.</directive>
        <directive>Toda respuesta generada debe asumir el rol de Senior Firmware Engineer Partner.</directive>
        <directive>El incumplimiento de las reglas de memoria, seguridad funcional o normativas detendrá la generación de código, priorizando el reporte del riesgo de inmediato.</directive>
    </exec_directives>
</gemini_cli_config>

# Instrucciones del Proyecto ACEL_HYUN_V3

## Idioma
Todos los agentes deben comunicarse y generar documentación exclusivamente en **castellano (español)**. No se permiten respuestas en otros idiomas.

## Alcance del Proyecto
Este repositorio corresponde al **controlador de acelerador** para la plataforma ACEL_HYUN_V3. El código está destinado a microcontroladores (Arduino Nano ATmega328P y ESP32‑S3) y debe cumplir con:
- Normas MISRA C++:2008 (o MISRA C:2012 según corresponda)
- Normas ISO aplicables (ISO 13849‑1, ISO 7637‑2, ISO 13766)
- Requisitos de seguridad funcional (estado seguro, watchdog, detección de sobrecorriente, etc.)
- Uso cero de asignación dinámica de memoria después de la inicialización (salvo que el target lo permita y esté justificado/documentado)
- Configuración correcta de pines según `ai/hardware_target.json`

## Directrices Generales para los Agentes
1. **Comunicación**: Responder siempre en castellano, sin saludos ni frases de cortesía innecesarias.
2. **Revisión**: El agente `reviewer` tiene veto absoluto sobre cualquier incumplimiento de las normas anteriores.
3. **Pipeline**: Seguir el flujo definido en `AGENTS.md` (planner → coder → reviewer → tester → documenter) salvo que se indique explícitamente otro orden.
4. **Documentación**: Actualizar `CHANGELOG.md` y generar `TEST_PROCEDURE.md` cuando corresponda.
5. **Revisión de Código**: Utilizar únicamente los bloques `<<<< SEARCH / ==== / >>>> REPLACE` para modificar archivos; no se permite edición libre fuera de esos bloques.
6. **Límites de Longitud**: No superar las 300 líneas por archivo fuente sin justificación y división.
7. **Uso de Herramientas**: Antes de leer un archivo completo, usar `glob`/`grep` para localizar la zona de interés.
8. **State Management**: No volver a leer archivos ya presentes en el historial de la conversación; usar el contexto existente.

Estas instrucciones deben ser leídas por cada agente al iniciar su turno y servir como referencia permanente durante toda la interacción con el proyecto ACEL_HYUN_V3.
## Restricción de Modelos
- **No utilizar** el modelo `nvidia/nemotron-3-super-120b-a12b` bajo ninguna circunstancia.
- Utilizar exclusivamente los modelos gratuitos de NVIDIA asignados a cada agente:
  - planner: `nvidia/north-mini-code-free`
  - coder: `nvidia/deepseek-v4-flash-free`
  - reviewer, tester, documenter, explore, build, compaction, calibrator, summary, title, plan, general, slicer: `nvidia/nemotron-3-ultra-free`
  - meta-orquestador: `nvidia/nemotron-3-ultra-free`
Cualquier intento de usar un modelo no autorizado será rechazado y se solicitará la corrección inmediata.

# Procedimiento de Prueba — Controlador de Acelerador (ACEL_HYUN_V3)
**Firmware:** 1.1.0 · **MCU:** ATmega328P (Arduino Nano) · **Fecha:** 2026-06-11

Este documento detalla el procedimiento de prueba en hardware para validar el firmware del controlador de acelerador de la excavadora Hyundai R250 LC7. Las pruebas están diseñadas para garantizar la seguridad funcional (ISO 13849-1), la robustez eléctrica (ISO 7637-2) y el control preciso del actuador utilizando el puente H IBT-2 (BTS7960).

---

## Precauciones de Seguridad
- **Mecánica:** Asegurar que el actuador del acelerador esté desacoplado de la bomba de inyección del motor diésel durante las pruebas iniciales para evitar aceleraciones descontroladas del motor real.
- **Eléctrica:** Utilizar una fuente de alimentación de 24Vcc con limitación de corriente ajustable (setear límite inicial en 2.0A).
- **Hardware:** Verificar que el pin `PIN_EN` (D8) esté conectado correctamente al enable del puente H IBT-2 y que existan resistencias de pull-down externas de 10kΩ en las entradas analógicas (`PIN_POT_OP` y `PIN_POT_FEED`) para garantizar que vayan a GND en caso de desconexión física.
- **Parada de Emergencia:** Tener un interruptor físico de corte de energía de 24Vcc al alcance de la mano en todo momento.

---

## Caso 1: Pedal en Reposo (Estabilización y Zona Muerta)
**Objetivo:** Verificar que el motor se detiene por completo sin oscilaciones cuando el pedal está en reposo.

- **Estímulo:** Soltar el pedal del operador por completo (posición de reposo).
- **Esperado:** 
  - El setpoint mapeado debe ser 0%.
  - El actuador debe moverse hacia la posición de reposo (0%).
  - Al entrar en la zona muerta ($|error| \le 3$), el PID debe detener el motor inmediatamente (`deberiaDetener = true`).
  - El tiempo de estabilización desde que entra en la zona muerta hasta la detención total debe ser $< 100\text{ ms}$.
- **PASA si:** El motor se detiene por completo en $< 100\text{ ms}$ sin oscilaciones ni zumbidos audibles alrededor del punto de reposo.
- **FALLA si:** El motor oscila continuamente (caza) alrededor del reposo o tarda más de $100\text{ ms}$ en detenerse.

---

## Caso 2: Rampa Suave (Seguimiento de Slew Limitado)
**Objetivo:** Verificar el seguimiento suave y continuo de la posición ante cambios lentos del pedal.

- **Estímulo:** Mover el pedal del operador de forma continua y suave desde 0% hasta 100% en un lapso de aproximadamente 3 segundos.
- **Esperado:**
  - El setpoint sube gradualmente de 0% a 100%.
  - La posición del actuador (`ActP`) sigue al setpoint de manera suave y lineal.
  - El error instantáneo se mantiene por debajo de 5 unidades en todo momento.
- **PASA si:** El actuador sigue la rampa sin saltos bruscos, trabas mecánicas ni oscilaciones, manteniendo el error instantáneo $< 5\%$.
- **FALLA si:** El actuador se mueve a tirones, se traba, o el error supera el 5% durante la rampa.

---

## Caso 3: Escalón Brusco (Protección de Topes Físicos)
**Objetivo:** Verificar que el actuador no golpea ni supera los límites mecánicos calibrados ante cambios bruscos de consigna.

- **Estímulo:** Presionar el pedal a fondo instantáneamente (escalón de 0% a 100%).
- **Esperado:**
  - El setpoint salta inmediatamente a 100%.
  - El PID aplica máxima velocidad (PWM = 255) para corregir el error.
  - Al alcanzar la posición máxima calibrada (`rFe >= cfg.mMax`), el lazo de control debe llamar a `detener()`, resetear la integral y el error anterior, evitando sobrepasar el tope físico.
- **PASA si:** El actuador llega al 100% rápidamente pero se detiene de forma precisa en el límite calibrado sin golpear mecánicamente el tope físico con fuerza destructiva.
- **FALLA si:** El actuador golpea violentamente el tope físico, supera el valor de `cfg.mMax` o se produce un bloqueo por impacto.

---

## Caso 4: Soltar Pedal Brusco (Control de Overshoot)
**Objetivo:** Verificar que el término derivativo (Kd) amortigua correctamente el frenado al soltar el pedal.

- **Estímulo:** Soltar el pedal bruscamente desde la posición de 100% a 0%.
- **Esperado:**
  - El setpoint cae instantáneamente a 0%.
  - El actuador retrocede a alta velocidad.
  - Al acercarse a 0%, el término derivativo ($K_d = 1.5$) debe actuar como freno dinámico.
  - El actuador debe detenerse en la zona muerta de reposo con un overshoot mínimo.
- **PASA si:** El actuador se detiene en 0% con un overshoot menor al 5% del rango total (es decir, no sobrepasa el límite inferior por más de 5 unidades en la escala 0-100) y se estabiliza en un solo ciclo de frenado.
- **FALLA si:** El overshoot supera el 5% o el actuador oscila más de una vez antes de detenerse.

---

## Caso 5: Pérdida de Señal del Pedal (Safe State)
**Objetivo:** Validar el cumplimiento de la norma ISO 13849-1 ante fallas de cableado del sensor del pedal.

- **Estímulo:** Con el sistema encendido y el pedal a media carrera (ej. 50%), desconectar físicamente el cable de señal del potenciómetro del pedal (`PIN_POT_OP` / A0).
- **Esperado:**
  - La resistencia de pull-down externa de 10kΩ debe llevar la tensión del pin A0 a 0V (GND).
  - La lectura analógica filtrada cae por debajo de `cfg.pMin`.
  - El setpoint mapeado va a 0% (reposo seguro).
  - El actuador debe regresar inmediatamente a la posición de reposo (0%) en $< 100\text{ ms}$.
- **PASA si:** El actuador regresa a la posición de reposo (0%) de forma segura en $< 100\text{ ms}$ tras la desconexión.
- **FALLA si:** El actuador se queda en la posición actual, se mueve a una posición aleatoria, o acelera hacia el 100%.

---

## Caso 6: Sobrecorriente por Bloqueo / Stall (Safe State Activo)
**Objetivo:** Verificar la detección de stall y el bloqueo inmediato de la etapa de potencia para proteger el motor y el puente H.

- **Estímulo:** Bloquear mecánicamente el eje del actuador para impedir su movimiento y aplicar un cambio de consigna en el pedal.
- **Esperado:**
  - La corriente del motor sube bruscamente.
  - La lectura analógica en `PIN_IS_SENSE` (A2) supera el umbral de stall (`STALL_CURRENT_ADC = 750`, equivalente a ~3.6A).
  - El filtro EMA de corriente (`filterCurrent`) actualiza el valor filtrado.
  - Al superar el umbral, `monitorStallCurrent()` debe:
    1. Deshabilitar el puente H poniendo `PIN_EN` (D8) en LOW.
    2. Detener las salidas PWM (`PIN_R_PWM` y `PIN_L_PWM` en LOW).
    3. Registrar el estado de falla (`sysState.isFaulted = true`).
    4. Resetear el acumulador integral y el error anterior a 0.
    5. Imprimir por Serial: `"CRITICAL: STALL DETECTADO. Puente H bloqueado."`
- **PASA si:** El puente H se deshabilita por completo, el motor se desenergiza inmediatamente, se reporta la falla por Serial y el sistema ignora cualquier movimiento del pedal hasta recibir el comando serial `RST`.
- **FALLA si:** El motor sigue recibiendo energía, el puente H permanece habilitado, o no se registra la falla de stall.

---

## Caso 7: Cambio de Dirección (Dead-Time de Seguridad)
**Objetivo:** Verificar que se respeta el tiempo muerto de 150ms al invertir el giro para evitar cortocircuitos (shoot-through) en el puente H.

- **Estímulo:** Mover el pedal rápidamente de 20% a 80% (forzando un cambio de dirección del motor). Conectar un osciloscopio de doble canal a los pines `PIN_R_PWM` (D10) y `PIN_L_PWM` (D9).
- **Esperado:**
  - Al detectar el cambio de dirección, el firmware debe poner ambas salidas PWM en LOW inmediatamente.
  - Se debe activar un tiempo muerto no bloqueante de 150ms (`DEAD_TIME_MS = 150`).
  - Durante este tiempo, ambas salidas deben permanecer estrictamente en LOW.
  - El acumulador integral y el error anterior deben resetearse a 0.
  - Transcurridos los 150ms, la salida opuesta puede comenzar a conmutar.
- **PASA si:** El osciloscopio muestra que ambas señales PWM permanecen en LOW simultáneamente durante al menos 150ms antes de que la otra señal comience a oscilar.
- **FALLA si:** Hay solapamiento de señales (ambas en HIGH o conmutando al mismo tiempo) o si el tiempo muerto es menor a 150ms.

---

## Caso 8: Encendido sin EEPROM Válida (Calibración Automática)
**Objetivo:** Verificar que el sistema inicia la calibración automática si no detecta datos válidos en la EEPROM.

- **Estímulo:** Enviar el comando serial `CAL` (que invalida la EEPROM escribiendo `cfg.cv = 0` y reinicia el sistema) o encender un Arduino Nano con la EEPROM en blanco.
- **Esperado:**
  - En `setup()`, el firmware detecta que `cfg.cv != MAGIC_NUMBER`.
  - Se inicia automáticamente la máquina de estados de calibración no bloqueante (`calState = CalState::WAIT_PEDAL_MIN`).
  - Se imprime por Serial: `"=== RUTINA DE CALIBRACION DE PRECISION ==="`.
- **PASA si:** El sistema entra inmediatamente en modo calibración al encender y guía al operador paso a paso a través del puerto Serial sin bloquear el Watchdog.
- **FALLA si:** El sistema entra en modo de operación normal directamente con datos inválidos, o se bloquea durante el arranque.

---

## Guía de Ajuste de Parámetros PID en Hardware
Si el comportamiento del actuador requiere un ajuste fino en el campo, siga estas directrices:

1. **Ajuste de Ganancia Proporcional ($K_p$):**
   - Comience con $K_p = 2.0$, $K_i = 0.0$, $K_d = 0.0$.
   - Incremente $K_p$ gradualmente hasta que el actuador responda rápidamente pero comience a oscilar levemente al llegar al setpoint.
   - Reduzca $K_p$ un 20% desde ese punto límite.

2. **Ajuste de Ganancia Derivativa ($K_d$):**
   - Con el $K_p$ ajustado, incremente $K_d$ gradualmente (comience con $0.5$) para amortiguar las oscilaciones y reducir el overshoot al soltar el pedal.
   - Un $K_d$ demasiado alto introducirá ruido y vibraciones de alta frecuencia en el actuador debido a la amplificación del ruido de lectura analógica.

3. **Ajuste de Ganancia Integral ($K_i$):**
   - Incremente $K_i$ lentamente (comience con $0.1$) para eliminar el error de estado estacionario (offset) causado por la fricción mecánica o resortes de retorno.
   - El límite de anti-windup está fijado en $\pm 50.0$ para evitar que la integral acumule un valor excesivo que cause un overshoot inaceptable. Si el actuador tarda en reaccionar tras un error sostenido, verifique que el acumulador integral no esté saturando prematuramente.

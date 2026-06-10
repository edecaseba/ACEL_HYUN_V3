<?xml version="1.0" encoding="UTF-8"?>
<hardware_control machinery="Excavadora Hyundai R250 LC7" application="Controlador de Acelerador" voltage="24Vcc">
    <bridge_h_strategy module="IBT_2_BTS7960">
        <switching_topology>Inyección obligatoria de tiempo muerto (dead-time) por software o hardware antes de conmutar entre los pines de avance (RPWM) y retroceso (LPWM) para mitigar el riesgo de shoot-through.</switching_topology>
        <freewheeling_mode>Especificar explícitamente el modo de rueda libre (parada por alta impedancia) o frenado activo (ambos canales en estado bajo/alto) según la dinámica requerida por el acelerador.</freewheeling_mode>
        <pwm_configuration>Frecuencia y resolución óptimas para el control de posición del acelerador que minimice el zumbido audible, optimice la disipación térmica del BTS7960 y mantenga inmunidad a perturbaciones EMI.</pwm_configuration>
    </bridge_h_strategy>
    <algorithms>
        <auto_calibration>Rutina no bloqueante post-inicialización para determinar de forma autónoma los límites físicos (topes mecánicos) del acelerador de la excavadora mediante la retroalimentación de posición.</auto_calibration>
        <pid_controller>Implementación en tiempo discreto utilizando exclusivamente aritmética de punto flotante de precisión simple (float) o punto fijo. Cálculo determinista con tiempo de muestreo constante (Ts) y protección anti-windup obligatoria.</pid_controller>
    </algorithms>
</hardware_control>
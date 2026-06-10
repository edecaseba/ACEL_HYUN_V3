<?xml version="1.0" encoding="UTF-8"?>
<safety_and_compliance>
    <normative_matrix>
        <iso_13849_1>
            <title>Seguridad de las máquinas - Partes de los sistemas de mando relativas a la seguridad</title>
            <requirement>Implementar Safe State ante fallos críticos. Ante pérdida de señal de sensores, timeout de control o sobrecorriente, todas las salidas de potencia deben pasar a estado seguro (0 lógico/físico o alta impedancia).</requirement>
        </iso_13849_1>
        <iso_7637_2>
            <title>Vehículos de carretera - Perturbaciones eléctricas por conducción y acoplamiento</title>
            <requirement>Protección estricta frente a transitorios automotrices de 24Vcc. El software debe soportar cranking y picos de tensión mediante filtrado digital avanzado (debouncing, filtros de media móvil o exponenciales) en lecturas analógicas.</requirement>
        </iso_7637_2>
        <iso_13766>
            <title>Maquinaria para movimiento de tierras - Compatibilidad electromagnética</title>
            <requirement>Minimizar EMI y ruido electromagnético. Gestionar frecuencias de PWM óptimas y evitar cambios abruptos de estado de potencia sin control de rampa o control de dead-time.</requirement>
        </iso_13766>
    </normative_matrix>
    <boot_security>
        <risk>Estados indeterminados o espurios de los pines GPIO durante el arranque del microcontrolador (fase de bootloader).</risk>
        <mitigation>Configurar explícitamente las direcciones de los pines de control y sus safe-states por defecto mediante registros de bajo nivel antes de activar las etapas de potencia.</mitigation>
    </boot_security>
</safety_and_compliance>
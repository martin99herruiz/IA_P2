# AGENTS.md — Nivel 6

## Contexto
Práctica 2 IA UGR: Operación Contención Belkanita.

Nivel actual: NIVEL 6.

El nivel 6 es mixto:
- parte reactiva: explorar mapa desconocido
- parte deliberativa: planificar movimientos cuando haya información suficiente
- coordinación Ingeniero/Técnico

## Estado actual
Nivel 6: 6/12 PASSED (regresión del commit "Intento de afianar").

Fallan actualmente (tras aplicar fixes parciales):
- T6.4: Task not finished (nueva regresión)
- T6.5: Se agotaron instantes (nueva regresión)
- T6.8: Se agotaron instantes (nueva regresión)
- T6.9: Task not finished (fallo original)
- T6.11: Se agotaron instantes
- T6.12: Se agotaron instantes

Fixes aplicados en este ciclo:
- Restaurado 'S' en tecnico.cpp::es_camino (era regresión del commit anterior)
- Revertido bloque conflictoFrontal/choqueTrasAvanzar en ComportamientoIngenieroNivel_0
- ComportamientoTecnicoNivel_6 ya tenía return IDLE (sin commitear)

## Objetivo
Mejorar SOLO nivel 6 sin romper los tests que ya pasan.

## Reglas obligatorias
- Trabajar SOLO sobre nivel 6.
- No tocar niveles 0, 1, 2, 3, 4 ni 5.
- No modificar funciones compartidas salvo justificación previa.
- No reestructurar el proyecto.
- No cambiar nombres de clases, métodos, structs ni enums.
- No inventar acciones.
- Cambios mínimos y localizados.
- No repetir cambios que ya hayan empeorado otros niveles.

## Archivos permitidos
Preferentemente:
- `ingeniero.cpp`
- `ingeniero.hpp`
- `tecnico.cpp`
- `tecnico.hpp`

Pero solo en funciones del nivel 6:
- `ComportamientoIngenieroNivel_6`
- `ComportamientoTecnicoNivel_6`

## Diagnóstico actual
Los fallos son de dos tipos:
- `Task not finished`: los agentes no completan la misión.
- `CPU TIME EXCEEDED`: alguna búsqueda o replanteamiento está explotando en tiempo.

## Reglas específicas nivel 6
- El mapa puede ser desconocido.
- No asumir mapa completo.
- Usar `mapaResultado` y `mapaCotas` como memoria parcial.
- Evitar planificar sobre demasiadas casillas `?`.
- Evitar recalcular planes cada tick.
- Si hay un plan válido, ejecutarlo.
- Solo replantear cuando:
  - el plan se acaba,
  - aparece un obstáculo,
  - hay choque,
  - el destino cambia,
  - o el plan lleva muchos pasos sin progreso.
- Añadir límite de expansión a las búsquedas para evitar CPU TIME.
- Si la búsqueda falla, volver a exploración reactiva segura.

## Prioridad de trabajo
1. Primero arreglar `CPU TIME EXCEEDED` en T6.12.
2. Después mejorar `Task not finished` en T6.9 y T6.11.
3. No sacrificar tests que ya pasan.

## Prohibido

- No modificar nivel 5.
- No modificar nivel 0.
- No añadir cambios globales de transitabilidad sin justificar.
- No tocar la lógica de niveles 2, 3 o 4, que ya pasan.

## Flujo obligatorio
Antes de modificar código, responder con:

### Diagnóstico
### Patrón detectado
### Causa probable
### Archivo y función
### Cambio mínimo propuesto
### Riesgo para tests que ya pasan
### Cómo probar

No aplicar varios cambios a la vez.
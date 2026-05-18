// =============================================================================
// TECNICO.CPP — Comportamiento del agente Técnico
// =============================================================================
//
// VISIÓN GENERAL
// El Técnico navega un mapa 2D con obstáculos, alturas y niebla de guerra.
// Cada turno recibe un struct Sensores y devuelve una Action.
//
// MAPA DE SENSORES (superficie[0..15], cota[0..15], agentes[0..15])
//   El agente ve un triángulo de 15 casillas delante de él + la suya propia:
//     [0]  = casilla actual
//     [1,2,3] = fila a distancia 1 (izq, centro, der respecto al rumbo)
//     [4..8]  = fila a distancia 2
//     [9..15] = fila a distancia 3
//   Los índices de agentes[k] = 'i' significan que el Ingeniero ocupa esa casilla.
//
// TIPOS DE SUPERFICIE
//   'C' camino normal    'S' superficie especial   'D' zapatillas mágicas
//   'U' meta (objetivo)  'B' bosque (sólo con zap)
//   'P' precipicio       'M' muro
//   '?' casilla aún no descubierta (niebla)
//
// ALTURA (cota[k])
//   Número entero [1..9]. El técnico sólo puede moverse si |Δcota| <= 1.
//   Con zapatillas ('D') el ingeniero tolera |Δcota| <= 2, pero el TÉCNICO
//   siempre está limitado a 1, independientemente de las zapatillas.
//
// ACCIONES DISPONIBLES
//   WALK      Avanzar a la casilla de enfrente
//   TURN_SL   Girar 45° a la izquierda
//   TURN_SR   Girar 45° a la derecha
//   IDLE      No hacer nada (coste 0)
//   INSTALL   Instalar tramo de tubería (sólo en N5/N6)
//
// ARQUITECTURA POR NIVELES
//   N0: Reactivo + mano izquierda + A* cuando el mapa es suficientemente conocido
//   N1: Reactivo con puntuación por altura + sistema anti-bucle
//   N2: Pasivo — se aparta si bloquea la casilla de la Belkanita
//   N3: Deliberativo puro — A* hasta la posición de la Belkanita
//   N4: No implementado (IDLE)
//   N5: Máquina de estados — recibe COME del Ingeniero, va al punto, instala
//   N6: Extiende N5 — si está libre se acerca a la Belkanita con greedy
//
// =============================================================================

#include "tecnico.hpp"
#include "motorlib/util.h"
#include <algorithm>
#include <iostream>
#include <queue>
#include <set>
#include <map>

using namespace std;

namespace
{
  constexpr bool DEBUG_N5_TRACE_TEC = false;

  const char *NombreEstadoT5(int st)
  {
    switch (st)
    {
    case 0:
      return "LIBRE";
    case 1:
      return "YENDO_OBJETIVO";
    case 2:
      return "ESPERANDO_INSTALL";
    default:
      return "?";
    }
  }
} // namespace

// =========================================================================
// ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
// =========================================================================
// =============================================================================
// FUNCIÓN: CoordenadaSensor123T
// Convierte el índice de sensor (1=izquierda, 2=centro, 3=derecha) a
// coordenadas absolutas del mapa (fila, columna).
// La dirección "izquierda/centro/derecha" es RELATIVA al rumbo del agente:
//   - índice 2 siempre es la casilla justo DELANTE
//   - índice 1 es la casilla diagonal-izquierda
//   - índice 3 es la casilla diagonal-derecha
// Esto permite evaluar los tres movimientos posibles sin girar primero.
// =============================================================================
pair<int, int> CoordenadaSensor123T(int f, int c, Orientacion brujula, int idx)
{
  switch (brujula)
  {
  case norte:
    if (idx == 1)
      return {f - 1, c - 1};
    if (idx == 2)
      return {f - 1, c};
    return {f - 1, c + 1};

  case noreste:
    if (idx == 1)
      return {f - 1, c};
    if (idx == 2)
      return {f - 1, c + 1};
    return {f, c + 1};

  case este:
    if (idx == 1)
      return {f - 1, c + 1};
    if (idx == 2)
      return {f, c + 1};
    return {f + 1, c + 1};

  case sureste:
    if (idx == 1)
      return {f, c + 1};
    if (idx == 2)
      return {f + 1, c + 1};
    return {f + 1, c};

  case sur:
    if (idx == 1)
      return {f + 1, c + 1};
    if (idx == 2)
      return {f + 1, c};
    return {f + 1, c - 1};

  case suroeste:
    if (idx == 1)
      return {f + 1, c};
    if (idx == 2)
      return {f + 1, c - 1};
    return {f, c - 1};

  case oeste:
    if (idx == 1)
      return {f + 1, c - 1};
    if (idx == 2)
      return {f, c - 1};
    return {f - 1, c - 1};

  case noroeste:
    if (idx == 1)
      return {f, c - 1};
    if (idx == 2)
      return {f - 1, c - 1};
    return {f - 1, c};
  }

  return {f, c};
}

// =============================================================================
// FUNCIÓN: EsTransitableNivel1T
// Devuelve true si el tipo de casilla permite el paso.
// El bosque ('B') sólo es transitable si el Técnico YA RECOGIÓ las zapatillas.
// Nota: el Técnico ignora 'A' (agua) y 'H' (hielo) porque no puede flotar
// ni deslizarse — esas casillas sólo las cruza el Ingeniero.
// =============================================================================
bool EsTransitableNivel1T(char casilla, bool zap)
{
  if (casilla == 'C' || casilla == 'S' || casilla == 'D' || casilla == 'U')
    return true;

  if (casilla == 'B' && zap)
    return true;

  return false;
}

// =============================================================================
// FUNCIÓN: PuntuarCasillaNivel1T
// Sistema de puntuación para elegir entre las tres casillas candidatas (N1).
// Lógica de puntuación:
//   +120  si la casilla era '?' en el mapa → bonus por explorar terreno nuevo
//   +50   si es 'C' o 'S'  → terrenos cómodos de recorrer
//   +15   si es 'D' o 'U'  → zapatillas o meta
//   +20   si es 'B' con zapatillas → bosque accesible
//   -5*v  por cada visita previa (desincentivar revisitar)
//   -20   penalización extra si se ha visitado >5 veces (bucle detectado)
//   -50   penalización MUY fuerte si es la casilla de la que venimos
//         (evita el rebote inmediato adelante-atrás)
// Retorna -100000 si la casilla no es transitable (no eligible).
// =============================================================================
int PuntuarCasillaNivel1T(char visible, unsigned char conocida, int visitas,
                          int f, int c, int ultimaF, int ultimaC, bool zap)
{
  if (!EsTransitableNivel1T(visible, zap))
    return -100000;

  int score = 0;

  if (conocida == '?')
    score += 120;

  if (visible == 'C' || visible == 'S')
    score += 50;

  if (visible == 'D' || visible == 'U')
    score += 15;

  if (visible == 'B' && zap)
    score += 20;

  score -= 5 * visitas;

  if (visitas > 5)
    score -= 20;

  if (f == ultimaF && c == ultimaC)
    score -= 50;

  return score;
}

// =============================================================================
// FUNCIÓN: ViablePorAlturaT
// Filtra una casilla por diferencia de cota.
// Parámetros:
//   casilla: tipo de superficie leído del sensor
//   dif:     cota[k] - cota[0] (positivo = la casilla está más alta)
//   zap:     si el Técnico lleva zapatillas (no afecta: límite siempre ±1)
// Retorna 'casilla' si el desnivel es aceptable, o 'P' (precipicio virtual)
// si es demasiado pronunciado.
// NOTA: el Técnico SIEMPRE tiene límite ±1, a diferencia del Ingeniero que
// con zapatillas puede hacer ±2. Por eso 'zap' no se usa aquí.
// Retornar 'P' en lugar de false permite componer con los demás filtros
// sin necesidad de otra variable de "casilla bloqueada".
// =============================================================================
char ViablePorAlturaT(char casilla, int dif, bool zap)
{
  if (abs(dif) <= 1)
    return casilla;
  else
    return 'P';
}

// =============================================================================
// FUNCIÓN: VeoCasillaInteresanteT
// Reactivo simple: dada la superficie de izquierda/centro/derecha inmediata,
// devuelve la dirección preferida hacia la casilla más interesante.
// Retorna: 1=izquierda, 2=centro, 3=derecha, 0=ninguna.
// Jerarquía de prioridad:
//   U (meta) > C (camino normal) > D (zapatillas)
// Dentro de cada tipo, el centro tiene prioridad sobre los laterales.
// Uso: en N0 como desempate cuando el sistema de puntuación no distingue.
// =============================================================================
int VeoCasillaInteresanteT(char i, char c, char d)
{
  if (c == 'U')
    return 2;
  if (i == 'U')
    return 1;
  if (d == 'U')
    return 3;

  if (c == 'C')
    return 2;
  if (i == 'C')
    return 1;
  if (d == 'C')
    return 3;

  if (c == 'D')
    return 2;
  if (i == 'D')
    return 1;
  if (d == 'D')
    return 3;

  return 0;
}

// =============================================================================
// FUNCIÓN: VeoULejoT
// Detecta si hay una casilla 'U' (meta) en la segunda o tercera fila del cono
// de visión y en qué mitad (izquierda o derecha) está.
// Retorna: 1=mitad izquierda, 3=mitad derecha, 0=no se ve U a distancia 2-3.
//
// Índices del sensor usados:
//   Fila 2 (distancia 2): [4]=izq extremo [5]=izq [6]=centro [7]=der [8]=der extremo
//   Fila 3 (distancia 3): [9]-[15]
// La mitad izquierda agrupa [4,5,9,10,11]; la derecha [7,8,13,14,15].
// El centro ([6,12]) no se contempla porque a esa distancia el eje centro
// ya lo cubre el sistema de puntuación de ElegirMovimientoNivel0T.
//
// Uso: fallback en N0 cuando ninguna de las 3 casillas inmediatas tiene
// puntuación positiva (callejón sin salida aparente). Orienta hacia la U
// más lejana para salir del bucle local.
// =============================================================================
int VeoULejoT(const Sensores &sensores)
{
  if (sensores.superficie[4] == 'U' || sensores.superficie[5] == 'U' ||
      sensores.superficie[9] == 'U' || sensores.superficie[10] == 'U' || sensores.superficie[11] == 'U')
    return 1;

  if (sensores.superficie[7] == 'U' || sensores.superficie[8] == 'U' ||
      sensores.superficie[13] == 'U' || sensores.superficie[14] == 'U' || sensores.superficie[15] == 'U')
    return 3;

  return 0;
}

Action ComportamientoTecnico::think(Sensores sensores)
{
  Action accion = IDLE;

  // Decisión del agente según el nivel
  switch (sensores.nivel)
  {
  case 0:
    accion = ComportamientoTecnicoNivel_0(sensores);
    break;
  case 1:
    accion = ComportamientoTecnicoNivel_1(sensores);
    break;
  case 2:
    accion = ComportamientoTecnicoNivel_2(sensores);
    break;
  case 3:
    accion = ComportamientoTecnicoNivel_3(sensores);
    break;
  case 4:
    accion = ComportamientoTecnicoNivel_4(sensores);
    break;
  case 5:
    accion = ComportamientoTecnicoNivel_5(sensores);
    break;
  case 6:
    accion = ComportamientoTecnicoNivel_6(sensores);
    break;
  }

  return accion;
}

// =============================================================================
// FUNCIÓN: EsCaminoNivel0T
// Predicado mínimo para N0: ¿es la casilla un tipo que el Técnico puede pisar?
// En N0 no se tiene en cuenta la altura (se consulta aparte) ni las zapatillas
// (el bosque no se considera en N0). Solo C, S, D y U son "camino".
// =============================================================================
bool EsCaminoNivel0T(char x)
{
  return x == 'C' || x == 'S' || x == 'D' || x == 'U';
}

// =============================================================================
// FUNCIÓN: ElegirMovimientoNivel0T
// Sistema de puntuación para N0: decide entre avanzar (2), girar izquierda (1)
// o girar derecha (3) basándose en las casillas i/c/d ya filtradas por altura.
//
// PUNTUACIÓN de cada candidata:
//   INF_NEG  si la casilla no es transitable (ya filtrada antes de llamar)
//   -60*vis  penalización base por revisitas (lineal)
//   -100     si ya visitada >=2 veces
//   -200     si ya visitada >=4 veces
//   -400     si ya visitada >=6 veces (doble capa anti-bucle)
//   -500     si coincide con la casilla de la que venimos (evita rebote)
//   +40      bonus si es el centro (continuar recto es más eficiente)
//   +70      si es 'C' / +10 si es 'S'
//   -5       si es lateral Y el rumbo es diagonal (estabiliza el giro)
//   +300/+180 si es 'U' (meta): centro paga más que lateral
//   +80      si es 'D' (zapatillas)
//
// DESEMPATE:
//   Centro > resto; si empate con centro → avanzar si no es la posición anterior.
//   Si I==D → elegir la menos visitada; si igual → mano_derecha desempata.
//   mano_derecha: el Técnico usa false (prefiere izquierda) para explorar
//   en sentido contrario al Ingeniero (que usa true).
//
// Retorna: 2=WALK, 1=TURN_SL, 3=TURN_SR, 0=ninguna opción válida.
// =============================================================================
int ElegirMovimientoNivel0T(const Sensores &sensores, char i, char c, char d, const vector<vector<int>> &mapaVisitas, int ultimaFila, int ultimaCol, bool mano_derecha)
{
  pair<int, int> pi = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 1);
  pair<int, int> pc = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 2);
  pair<int, int> pd = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 3);

  const int INF_NEG = -1000000;

  auto enRango = [&](pair<int, int> p)
  {
    return p.first >= 0 && p.first < (int)mapaVisitas.size() &&
           p.second >= 0 && p.second < (int)mapaVisitas[0].size();
  };

  auto puntuar = [&](char casilla, pair<int, int> p, bool esCentro)
  {
    if (!(casilla == 'C' || casilla == 'S' || casilla == 'D' || casilla == 'U'))
      return INF_NEG;
    if (!enRango(p))
      return INF_NEG;

    int vis = mapaVisitas[p.first][p.second];
    int score = 0;

    // Penalización fuerte por revisitar
    score -= 60 * vis;

    if (vis >= 2)
      score -= 100;
    if (vis >= 4)
      score -= 200;
    if (vis >= 6)
      score -= 400;

    // Penalización MUY fuerte por volver justo atrás
    if (p.first == ultimaFila && p.second == ultimaCol)
      score -= 500;

    // Bonus por seguir recto
    if (esCentro)
      score += 40;

    if (casilla == 'C')
      score += 70;
    else if (casilla == 'S')
      score += 10;

    // Penalizar un poco las vibraciones en diagonal
    if (!esCentro && (sensores.rumbo % 2 != 0))
      score -= 5;

    // U delante sigue siendo muy atractiva, pero no infinita
    if (casilla == 'U')
      score += esCentro ? 300 : 180;

    // D es útil, pero menos que U
    if (casilla == 'D')
      score += 80;

    return score;
  };

  int scoreI = puntuar(i, pi, false);
  int scoreC = puntuar(c, pc, true);
  int scoreD = puntuar(d, pd, false);

  // Si el centro es claramente mejor, avanzar
  if (scoreC > scoreI && scoreC > scoreD && scoreC > INF_NEG)
    return 2;

  // Empates con el centro: favorecer avanzar si no es volver atrás
  if (scoreC == scoreI && scoreC > scoreD && scoreC > INF_NEG)
  {
    if (!(pc.first == ultimaFila && pc.second == ultimaCol))
      return 2;
  }

  if (scoreC == scoreD && scoreC > scoreI && scoreC > INF_NEG)
  {
    if (!(pc.first == ultimaFila && pc.second == ultimaCol))
      return 2;
  }

  if (scoreC == scoreI && scoreC == scoreD && scoreC > INF_NEG)
  {
    if (!(pc.first == ultimaFila && pc.second == ultimaCol))
      return 2;
  }

  // Desempate izquierda/derecha por menos visitas
  if (scoreI == scoreD && scoreI > INF_NEG)
  {
    int visI = enRango(pi) ? mapaVisitas[pi.first][pi.second] : 999999;
    int visD = enRango(pd) ? mapaVisitas[pd.first][pd.second] : 999999;

    if (visI < visD)
      return 1;
    if (visD < visI)
      return 3;

    return mano_derecha ? 3 : 1;
  }

  if (mano_derecha)
  {
    if (scoreD > scoreI && scoreD > INF_NEG)
      return 3;
    if (scoreI > INF_NEG)
      return 1;
  }
  else
  {
    if (scoreI > scoreD && scoreI > INF_NEG)
      return 1;
    if (scoreD > INF_NEG)
      return 3;
  }

  if (scoreC > INF_NEG)
    return 2;

  return 0;
}
// =============================================================================
// NIVEL 0 — Reactivo con mano izquierda + A* cuando el mapa está descubierto
// =============================================================================
// Este nivel combina dos estrategias:
//
//   FASE REACTIVA (siempre activa):
//     - Navega por "mano izquierda": al llegar a un cruce prefiere girar a la
//       izquierda (mano_derecha=false). Esto no garantiza encontrar la meta pero
//       en mapas con pasillos es suficientemente eficiente.
//     - Usa ElegirMovimientoNivel0T con un sistema de puntuación basado en
//       penalizar casillas revisitadas y premiar explorar terreno nuevo.
//     - Anti-bucle: invierte la mano cada 20 visitas a la misma casilla (>35 veces).
//     - Anti-atasco: si lleva 6 turnos sin moverse, activa plan_escape=2 que
//       fuerza 2 giros TURN_SL para romper el patrón local.
//
//   FASE DELIBERATIVA (cuando mapaResultado.size() >= 90):
//     - Cuando el mapa tiene al menos 90 filas conocidas, busca las casillas 'U'
//       (objetivos) y lanza A* hasta la más cercana.
//     - Intenta hasta 6 objetivos distintos si el primero no tiene camino.
//     - Límite de 5000 expansiones para no bloquear el hilo.
//     - Si el Técnico ya tiene zapatillas, puede traversar bosques en el A*.
//
//   ORDEN DE PRIORIDAD DE ACCIONES:
//     1. Energía <= 12 → IDLE (preservar batería)
//     2. Sobre 'U' → IDLE (ya llegó)
//     3. Plan A* disponible → ejecutar siguiente paso
//     4. Casillas 'U' inmediatas a la vista → moverse hacia ellas
//     5. ElegirMovimientoNivel0T → sistema de puntuación
//     6. VeoULejoT → U en distancia 2-3 → orientarse hacia ella
//     7. Fallback → girar según mano
// =============================================================================

Action ComportamientoTecnico::ComportamientoTecnicoNivel_0(Sensores sensores)
{
  Action accion = IDLE;

  // =========================================================
  // 1. Fase de observación / actualización del mapa y visitas
  // =========================================================
  ActualizarMapa(sensores);
  mapaVisitas[sensores.posF][sensores.posC]++;

  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;

  if (sensores.energia <= 12)
  {
    last_action = IDLE;
    ultimaPosF = sensores.posF;
    ultimaPosC = sensores.posC;
    return IDLE;
  }

  // Detectar si el técnico lleva varios turnos sin avanzar
  bool mismaPosicion = (sensores.posF == ultimaPosF && sensores.posC == ultimaPosC);

  if (mismaPosicion && last_action != IDLE)
    turnos_sin_avanzar++;
  else
    turnos_sin_avanzar = 0;

  // Si chocó al avanzar o lleva demasiado tiempo atascado, activar escape
  if ((sensores.choque && last_action == WALK) || turnos_sin_avanzar >= 6)
  {
    plan_escape = 2;
    turnos_sin_avanzar = 0;
  }

  // Modo escape: durante unos turnos rompe el patrón local
  if (plan_escape > 0)
  {
    plan_escape--;

    // Como el técnico usa mano izquierda, rompe el atasco girando a la izquierda
    accion = TURN_SL;

    last_action = accion;
    ultimaPosF = sensores.posF;
    ultimaPosC = sensores.posC;
    return accion;
  }

  // Si llegué a planta, me quedo quieto
  // if (sensores.superficie[0] == 'U')
  //  accion = IDLE;

  if (sensores.superficie[0] == 'U')
  {
    hayPlan = false;
    plan.clear();
    last_action = IDLE;
    ultimaPosF = sensores.posF;
    ultimaPosC = sensores.posC;
    return IDLE;
  }

  if (sensores.choque)
  {
    hayPlan = false;
    plan.clear();
  }

  if (hayPlan && !plan.empty())
  {
    Action sig = plan.front();
    if (sig != WALK || sensores.agentes[2] != 'i')
    {
      plan.pop_front();
      if (plan.empty())
        hayPlan = false;
      last_action = sig;
      ultimaPosF = sensores.posF;
      ultimaPosC = sensores.posC;
      return sig;
    }

    hayPlan = false;
    plan.clear();
  }

  vector<pair<int, int>> objetivos_u;
  for (int f = 0; f < (int)mapaResultado.size(); f++)
    for (int col = 0; col < (int)mapaResultado[f].size(); col++)
      if (mapaResultado[f][col] == 'U')
        objetivos_u.push_back({f, col});

  if (!objetivos_u.empty() && mapaResultado.size() >= 90)
  {
    sort(objetivos_u.begin(), objetivos_u.end(), [&](const pair<int, int> &a, const pair<int, int> &b)
         {
           int da = abs(a.first - sensores.posF) + abs(a.second - sensores.posC);
           int db = abs(b.first - sensores.posF) + abs(b.second - sensores.posC);
           return da < db;
         });

    EstadoT inicio, fin;
    inicio.site.f = sensores.posF;
    inicio.site.c = sensores.posC;
    inicio.site.brujula = sensores.rumbo;
    inicio.zapatillas = tiene_zapatillas;

    int intentos = min((int)objetivos_u.size(), 6);
    for (int k = 0; k < intentos && plan.empty(); k++)
    {
      fin.site.f = objetivos_u[k].first;
      fin.site.c = objetivos_u[k].second;
      fin.site.brujula = sensores.rumbo;
      fin.zapatillas = false;
      plan = AEstrellaTecnico(inicio, fin, 5000);
    }

    hayPlan = !plan.empty();
    if (hayPlan)
    {
      Action sig = plan.front();
      if (sig != WALK || sensores.agentes[2] != 'i')
      {
        plan.pop_front();
        if (plan.empty())
          hayPlan = false;
        last_action = sig;
        ultimaPosF = sensores.posF;
        ultimaPosC = sensores.posC;
        return sig;
      }

      hayPlan = false;
      plan.clear();
    }
  }

  // Salvaguarda suave contra bucles largos: invertir mano solo en revisita alta.
  if (mapaVisitas[sensores.posF][sensores.posC] > 35 &&
      (mapaVisitas[sensores.posF][sensores.posC] % 20) == 0)
  {
    mano_derecha = !mano_derecha;
  }
  // =========================================================
  // 2. Construcción de las 3 opciones inmediatas (izq-centro-der)
  //    usando más los métodos del profesor
  // =========================================================
  pair<int, int> pi = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 1);
  pair<int, int> pc = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 2);
  pair<int, int> pd = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 3);

  auto enRangoMapa = [&](pair<int, int> p)
  {
    return p.first >= 0 && p.first < (int)mapaResultado.size() &&
           p.second >= 0 && p.second < (int)mapaResultado[0].size();
  };

  auto viableLateral = [&](pair<int, int> p, char casillaSensor, int idxSensor)
  {
    if (!enRangoMapa(p))
      return 'P';

    // El método del profesor comprueba tránsito en nivel 0
    if (!EsCasillaTransitableLevel0(p.first, p.second, tiene_zapatillas))
      return 'P';

    // Para el Técnico el desnivel máximo es siempre 1
    int dif = sensores.cota[idxSensor] - sensores.cota[0];
    if (abs(dif) > 1)
      return 'P';

    return casillaSensor;
  };

  // Centro: aprovechamos la idea del método del profesor
  ubicacion actual;
  actual.f = sensores.posF;
  actual.c = sensores.posC;
  actual.brujula = sensores.rumbo;

  char c = 'P';
  if (EsAccesiblePorAltura(actual) &&
      enRangoMapa(pc) &&
      EsCasillaTransitableLevel0(pc.first, pc.second, tiene_zapatillas))
  {
    c = sensores.superficie[2];
  }

  // Izquierda y derecha: misma lógica, calculadas con su coordenada
  char i = viableLateral(pi, sensores.superficie[1], 1);
  char d = viableLateral(pd, sensores.superficie[3], 3);

  // =========================================================
  // 3. Reglas reactivas de conflicto
  // =========================================================

  bool ingenieroIzq = (sensores.agentes[1] == 'i');
  bool ingenieroCen = (sensores.agentes[2] == 'i');
  bool ingenieroDer = (sensores.agentes[3] == 'i');

  if (ingenieroIzq)
    i = 'P';
  if (ingenieroCen)
    c = 'P';
  if (ingenieroDer)
    d = 'P';

  if (sensores.choque && last_action == WALK)
  {
    accion = mano_derecha ? TURN_SR : TURN_SL;
    last_action = accion;
    ultimaPosF = sensores.posF;
    ultimaPosC = sensores.posC;
    return accion;
  }

  bool mismaPos = (sensores.posF == ultimaPosF && sensores.posC == ultimaPosC);

  // =========================================================
  // 4. Decisión reactiva
  // =========================================================

  // Las U inmediatas mandan
  if (c == 'U')
    accion = WALK;
  else if (i == 'U')
    accion = TURN_SL;
  else if (d == 'U')
    accion = TURN_SR;
  else
  {
    int decision = ElegirMovimientoNivel0T(sensores, i, c, d,
                                           mapaVisitas,
                                           ultimaFila, ultimaCol,
                                           mano_derecha);

    switch (decision)
    {
    case 2:
      accion = WALK;
      break;
    case 1:
      accion = TURN_SL;
      break;
    case 3:
      accion = TURN_SR;
      break;
    default:
    {
      // La U lejana solo orienta si no hay mejor opción local
      int posU = VeoULejoT(sensores);

      if (posU == 1)
        accion = TURN_SL;
      else if (posU == 3)
        accion = TURN_SR;
      else
        accion = mano_derecha ? TURN_SR : TURN_SL;

      break;
    }
    }

    // Antibucle simple
    if (mismaPos)
    {
      if (accion == TURN_SR || accion == TURN_SL)
        giros_consecutivos++;
      else
        giros_consecutivos = 0;
    }
    else
    {
      giros_consecutivos = 0;
    }

    if (giros_consecutivos >= 4)
    {
      accion = (last_action == TURN_SR) ? TURN_SL : TURN_SR;
      giros_consecutivos = 0;
    }
  }

  // =========================================================
  // 5. Actualización de memoria
  // =========================================================
  if (accion == WALK)
  {
    ultimaFila = sensores.posF;
    ultimaCol = sensores.posC;
  }

  last_action = accion;
  ultimaPosF = sensores.posF;
  ultimaPosC = sensores.posC;
  return accion;
}

/**
 * @brief Comprueba si una celda es de tipo camino transitable.
 * @param c Carácter que representa el tipo de superficie.
 * @return true si es camino ('C'), zapatillas ('D') o meta ('U').
 */
bool ComportamientoTecnico::es_camino(unsigned char c) const
{
  return (c == 'C' || c == 'S' || c == 'D' || c == 'U');
}

// =============================================================================
// NIVEL 1 — Reactivo con altura y sistema de puntuación
// =============================================================================
// Diferencias respecto a N0:
//   - Usa ViablePorAlturaT: si |Δcota| > 1 la casilla se marca 'P' (impasable)
//   - Usa PuntuarCasillaNivel1T: elige la casilla con MAYOR puntuación
//   - No hay A* ni búsqueda deliberativa, sólo decisión local cada turno
//
// Flujo principal:
//   1. Detección de atasco: si lleva >=4 turnos sin moverse, fuerza el último giro
//   2. Calcula las 3 casillas candidatas (izq/centro/der) con altura y agentes
//   3. Puntúa cada candidata con PuntuarCasillaNivel1T
//   4. Elige la acción que maximiza la puntuación
//   5. En caso de choque, elige el giro que apunta al candidato con más score
// =============================================================================
Action ComportamientoTecnico::ComportamientoTecnicoNivel_1(Sensores sensores)
{
  Action accion = IDLE;

  bool mismaCasilla = (sensores.posF == ultimaPosF && sensores.posC == ultimaPosC);

  if (mismaCasilla && last_action != WALK)
    turnos_sin_avanzar++;
  else
    turnos_sin_avanzar = 0;

  if (turnos_sin_avanzar >= 4)
  {
    if (last_action == TURN_SL || last_action == TURN_SR)
      accion = last_action;
    else
      accion = TURN_SL;

    last_action = accion;
    ultimaPosF = sensores.posF;
    ultimaPosC = sensores.posC;
    turnos_sin_avanzar = 0;
    return accion;
  }

  pair<int, int> pi = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 1);
  pair<int, int> pc = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 2);
  pair<int, int> pd = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 3);

  auto enRangoN1 = [&](pair<int, int> p)
  {
    return p.first >= 0 && p.first < (int)mapaResultado.size() &&
           p.second >= 0 && p.second < (int)mapaResultado[0].size();
  };

  unsigned char mi = enRangoN1(pi) ? mapaResultado[pi.first][pi.second] : 'P';
  unsigned char mc = enRangoN1(pc) ? mapaResultado[pc.first][pc.second] : 'P';
  unsigned char md = enRangoN1(pd) ? mapaResultado[pd.first][pd.second] : 'P';

  int vi = enRangoN1(pi) ? mapaVisitas[pi.first][pi.second] : 999999;
  int vc = enRangoN1(pc) ? mapaVisitas[pc.first][pc.second] : 999999;
  int vd = enRangoN1(pd) ? mapaVisitas[pd.first][pd.second] : 999999;

  ActualizarMapa(sensores);
  mapaVisitas[sensores.posF][sensores.posC]++;

  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;

  char i = ViablePorAlturaT(sensores.superficie[1],
                            sensores.cota[1] - sensores.cota[0],
                            tiene_zapatillas);

  char c = ViablePorAlturaT(sensores.superficie[2],
                            sensores.cota[2] - sensores.cota[0],
                            tiene_zapatillas);

  char d = ViablePorAlturaT(sensores.superficie[3],
                            sensores.cota[3] - sensores.cota[0],
                            tiene_zapatillas);

  // Evitar chocar con el Ingeniero
  if (sensores.agentes[1] == 'i')
    i = 'P';
  if (sensores.agentes[2] == 'i')
    c = 'P';
  if (sensores.agentes[3] == 'i')
    d = 'P';

  int scoreI = PuntuarCasillaNivel1T(i, mi, vi, pi.first, pi.second, ultimaFila, ultimaCol, tiene_zapatillas);
  int scoreC = PuntuarCasillaNivel1T(c, mc, vc, pc.first, pc.second, ultimaFila, ultimaCol, tiene_zapatillas);
  int scoreD = PuntuarCasillaNivel1T(d, md, vd, pd.first, pd.second, ultimaFila, ultimaCol, tiene_zapatillas);

  if (sensores.choque)
  {
    if (scoreD > scoreI)
      accion = TURN_SR;
    else if (scoreI > scoreD)
      accion = TURN_SL;
    else
    {
      if (last_action == TURN_SL || last_action == TURN_SR)
        accion = last_action;
      else
        accion = TURN_SL;
    }
  }
  else
  {
    if (scoreC >= scoreI && scoreC >= scoreD && scoreC > -100000)
    {
      accion = WALK;
    }
    else if (scoreD > scoreI && scoreD > -100000)
    {
      accion = TURN_SR; // sesgo opuesto al Ingeniero
    }
    else if (scoreI > scoreD && scoreI > -100000)
    {
      accion = TURN_SL;
    }
    else
    {
      if (last_action == TURN_SL || last_action == TURN_SR)
        accion = last_action;
      else
        accion = TURN_SL;
    }
  }

  if (accion == WALK)
  {
    ultimaFila = sensores.posF;
    ultimaCol = sensores.posC;
  }

  ultimaPosF = sensores.posF;
  ultimaPosC = sensores.posC;
  last_action = accion;
  return accion;
}

// =============================================================================
// NIVEL 2 — Comportamiento pasivo: ceder el paso a la Belkanita
// =============================================================================
// En N2 la Belkanita ya sabe a dónde ir (el Ingeniero la lleva). El Técnico
// sólo tiene que NO obstruir su camino.
//
// Reglas:
//   1. Si el Técnico está ENCIMA de la casilla destino de la Belkanita,
//      intenta avanzar hacia delante para liberarla; si no puede, gira.
//   2. Si el Ingeniero está justo delante (agentes[2]=='i'), gira a la derecha
//      para dejarle paso.
//   3. En cualquier otro caso, IDLE (no gastar energía innecesariamente).
//
// Nota: en N2 la Belkanita puede estar en cualquier casilla transitable,
// por eso el Técnico comprueba (posF==BelPosF && posC==BelPosC).
// =============================================================================
Action ComportamientoTecnico::ComportamientoTecnicoNivel_2(Sensores sensores)
{
  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;

  ubicacion actual;
  actual.f = sensores.posF;
  actual.c = sensores.posC;
  actual.brujula = sensores.rumbo;

  ubicacion del = Delante(actual);

  bool ingenieroVisible = false;
  for (int k = 1; k < 16; k++)
  {
    if (sensores.agentes[k] == 'i')
    {
      ingenieroVisible = true;
      break;
    }
  }

  // En nivel 2 la Belkanita puede estar en cualquier casilla transitable.
  // Si el Técnico ocupa esa casilla objetivo, debe apartarse cuanto antes.
  if (sensores.posF == sensores.BelPosF && sensores.posC == sensores.BelPosC)
  {
    if (EsCasillaTransitableLevel0(del.f, del.c, tiene_zapatillas) &&
        EsAccesiblePorAltura(actual) &&
        sensores.agentes[2] != 'i')
    {
      return WALK;
    }

    return TURN_SR;
  }

  // Si no estoy sobre U, solo reacciono si realmente lo bloqueo delante
  if (sensores.agentes[2] == 'i')
    return TURN_SR;

  return IDLE;
}

// =============================================================================
// FUNCIONES AUXILIARES DEL ALGORITMO A* (usado en N3, N5 y N6)
// =============================================================================
// El Técnico usa A* (en lugar de BFS como el Ingeniero) porque el coste de
// moverse varía por tipo de terreno: agua, hielo, arena cuestan más que
// camino normal. A* con heurística admisible garantiza el camino óptimo en
// energía, lo que es crucial para que el Técnico no se quede sin batería.
//
// Estado del A*: EstadoT = {ubicacion{f,c,brujula}, zapatillas}
//   - La brújula forma parte del estado porque girar tiene coste
//   - Las zapatillas cambian qué casillas son transitables ('B')
//
// Heurística: max(|Δfila|, |Δcol|) → cota inferior admisible usando
//   movimientos diagonales en tablero de 8 direcciones.
//
// Coste por acción (CosteEnergiaTecnico):
//   TURN: terreno=='A' → 5, terreno=='H' → 2, resto → 1
//   WALK sobre 'A' → 60 (+5 subir, -2 bajar)
//         sobre 'H' → 6  (+5 subir, -2 bajar)
//         sobre 'S' → 3  (+5 subir, -2 bajar)
//         resto     → 1
// =============================================================================

// =============================================================================
// FUNCIÓN: EsDestino
// Condición de parada del A*: ¿ha llegado el estado actual al destino?
// NOTA: sólo compara (fila, columna). La orientación y las zapatillas al llegar
// NO importan: el destino es una posición, no un estado concreto.
// Si se quisiera exigir llegar con orientación fija (p.ej. mirando al norte),
// habría que añadir la comparación de brujula aquí.
// =============================================================================
bool ComportamientoTecnico::EsDestino(const EstadoT &st, const EstadoT &fin) const
{
  return st.site.f == fin.site.f && st.site.c == fin.site.c;
}

// =============================================================================
// FUNCIÓN: EsCasillaTransitableTecnico
// Versión "completa" de transitable para el A* (usa mapaResultado).
// Diferencias respecto a EsTransitableNivel1T:
//   - Acepta también '?' (desconocida) si el mapa aún no está completo
//     → en N3 el mapa es conocido, pero en N6 puede haber niebla
//   - Rechaza explícitamente 'P' (precipicio) y 'M' (muro)
//   - 'B' solo transitable con zapatillas
//   - NO acepta 'A' ni 'H': el Técnico no puede cruzar agua ni hielo
// =============================================================================
bool ComportamientoTecnico::EsCasillaTransitableTecnico(int f, int c, bool zapatillas) const
{
  if (f < 0 || f >= mapaResultado.size() || c < 0 || c >= mapaResultado[0].size())
    return false;

  unsigned char celda = mapaResultado[f][c];

  if (celda == 'P' || celda == 'M')
    return false;

  if (celda == 'B' && !zapatillas)
    return false;

  return true;
}

// =============================================================================
// FUNCIÓN: NextCasillaTecnico
// Calcula la casilla que quedaría al frente del agente si avanzara (WALK),
// sin verificar si es válida (eso lo hace EsAccionAplicableTecnico).
// Aplica el desplazamiento correspondiente a las 8 orientaciones posibles.
// Se usa tanto en EsAccionAplicableTecnico (para validar) como en ApplyTecnico
// (para actualizar el estado tras WALK).
// =============================================================================
ComportamientoTecnico::EstadoT ComportamientoTecnico::NextCasillaTecnico(const EstadoT &st) const
{
  EstadoT sig = st;

  switch (st.site.brujula)
  {
  case norte:
    sig.site.f--;
    break;
  case noreste:
    sig.site.f--;
    sig.site.c++;
    break;
  case este:
    sig.site.c++;
    break;
  case sureste:
    sig.site.f++;
    sig.site.c++;
    break;
  case sur:
    sig.site.f++;
    break;
  case suroeste:
    sig.site.f++;
    sig.site.c--;
    break;
  case oeste:
    sig.site.c--;
    break;
  case noroeste:
    sig.site.f--;
    sig.site.c--;
    break;
  }

  return sig;
}

// =============================================================================
// FUNCIÓN: EsAccionAplicableTecnico
// Punto de control ÚNICO del A*: determina si una acción es legal en un estado.
//
// TURN_SR / TURN_SL: siempre válidos (girar nunca falla).
//
// WALK: válido si Y SÓLO SI:
//   1. La casilla de delante (NextCasillaTecnico) es transitable por tipo de terreno.
//   2. La diferencia de cota con la casilla de delante es <= 1.
//      (El Técnico tiene límite FIJO de ±1, independientemente de zapatillas.)
//
// *** PUNTO CLAVE PARA MODIFICACIÓN EN EXAMEN ***
// Si el enunciado dice "el Técnico sólo puede moverse en X dirección",
// añadir aquí una comprobación extra sobre st.site.brujula antes del resto.
// Ejemplo (solo cardinal):
//   if (st.site.brujula==noreste||st.site.brujula==sureste||...) return false;
// Esto afecta automáticamente a N3, N5 y N6 sin tocar más código.
// =============================================================================
bool ComportamientoTecnico::EsAccionAplicableTecnico(Action accion, const EstadoT &st) const
{
  if (accion == TURN_SR || accion == TURN_SL)
    return true;

  if (accion == WALK)
  {
    EstadoT next = NextCasillaTecnico(st);

    if (!EsCasillaTransitableTecnico(next.site.f, next.site.c, st.zapatillas))
      return false;

    int diff = abs((int)mapaCotas[next.site.f][next.site.c] - (int)mapaCotas[st.site.f][st.site.c]);
    return diff <= 1;
  }

  return false;
}

// =============================================================================
// FUNCIÓN: ApplyTecnico
// Aplica una acción a un estado y devuelve el estado resultante.
// NO verifica si la acción es aplicable (llamar antes a EsAccionAplicableTecnico
// o asegurarse de que el estado es válido).
//
// WALK:  avanza si la acción es aplicable; si no (por error) devuelve estado igual.
//        Además: si la nueva casilla es 'D', activa zapatillas en el estado.
// TURN_SR: incrementa brujula en 1 (módulo 8) → giro horario 45°.
// TURN_SL: decrementa brujula en 1 (módulo 8 con +7 para evitar negativo).
//
// USO PRINCIPAL: en AEstrellaTecnico para generar estados hijo.
// USO EXTRA: simular el recorrido de un plan completo para obtener el estado
//   exacto al final (necesario para los waypoints del Ejercicio 1 del examen).
//   Ejemplo:
//     EstadoT estado_final = estado_inicio;
//     for (Action a : plan)
//         estado_final = ApplyTecnico(a, estado_final);
// =============================================================================
ComportamientoTecnico::EstadoT ComportamientoTecnico::ApplyTecnico(Action accion, const EstadoT &st) const
{
  EstadoT next = st;

  switch (accion)
  {
  case WALK:
    if (EsAccionAplicableTecnico(WALK, st))
    {
      next = NextCasillaTecnico(st);
      if (mapaResultado[next.site.f][next.site.c] == 'D')
        next.zapatillas = true;
    }
    break;

  case TURN_SR:
    next.site.brujula = (Orientacion)(((int)next.site.brujula + 1) % 8);
    break;

  case TURN_SL:
    next.site.brujula = (Orientacion)(((int)next.site.brujula + 7) % 8);
    break;

  default:
    break;
  }

  return next;
}

// =============================================================================
// FUNCIÓN: CosteEnergiaTecnico
// Devuelve el coste en energía de ejecutar una acción desde el estado st.
// El coste depende del TERRENO DE LA CASILLA ACTUAL (st), no de la destino.
//
// GIRAR (TURN_SR/TURN_SL):
//   Agua  ('A') → 5    (difícil maniobrar en agua)
//   Hielo ('H') → 2    (deslizarse cuesta menos)
//   Resto       → 1
//
// AVANZAR (WALK) — también depende del delta de altura:
//   Agua  ('A') → 60 base (+5 si sube, -2 si baja)
//   Hielo ('H') → 6  base (+5 si sube, -2 si baja)
//   Arena ('S') → 3  base (+5 si sube, -2 si baja)
//   Resto (C,D,U,B,...) → 1 fijo
//
// Esta función hace que A* minimice ENERGÍA real, no solo número de pasos.
// Consecuencia: el A* puede preferir un camino más largo en casillas baratas
// (C/D) antes que uno corto pasando por agua o hielo.
// =============================================================================
int ComportamientoTecnico::CosteEnergiaTecnico(Action accion, const EstadoT &st) const
{
  unsigned char terreno = mapaResultado[st.site.f][st.site.c];

  if (accion == TURN_SR || accion == TURN_SL)
  {
    if (terreno == 'A')
      return 5;
    if (terreno == 'H')
      return 2;
    if (terreno == 'S')
      return 1;
    return 1;
  }

  if (accion == WALK)
  {
    EstadoT next = NextCasillaTecnico(st);
    int delta = (int)mapaCotas[next.site.f][next.site.c] - (int)mapaCotas[st.site.f][st.site.c];

    if (terreno == 'A')
    {
      int coste = 60;
      if (delta > 0)
        coste += 5;
      else if (delta < 0)
        coste -= 2;
      return coste;
    }
    else if (terreno == 'H')
    {
      int coste = 6;
      if (delta > 0)
        coste += 5;
      else if (delta < 0)
        coste -= 2;
      return coste;
    }
    else if (terreno == 'S')
    {
      int coste = 3;
      if (delta > 0)
        coste += 5;
      else if (delta < 0)
        coste -= 2;
      return coste;
    }
    else
    {
      // Resto: C, D, U, X, B con zapatillas, etc.
      return 1;
    }
  }

  return 0;
}

// =============================================================================
// FUNCIÓN: HeuristicaTecnico
// Heurística admisible para A*: distancia de Chebyshev × coste mínimo.
//
// max(|Δfila|, |Δcol|) es la distancia mínima en un tablero de 8 direcciones
// (Chebyshev): con movimientos diagonales puedes avanzar en ambos ejes a la vez.
// Como el coste mínimo de un WALK es 1, multiplicar por 1 da la heurística.
//
// Es ADMISIBLE porque nunca sobreestima:
//   - Se necesitan al menos max(df,dc) pasos incluso con el camino perfecto.
//   - Cada paso cuesta al menos 1.
//   → h(n) <= coste_real(n→destino) siempre.
//
// Es CONSISTENTE (monótona): h(n) <= coste(n→n') + h(n') para cualquier sucesor n'.
// Esto garantiza que A* con esta heurística nunca reabre nodos (graph-search óptimo).
//
// Si se quisiera una heurística más informada (p.ej. pesando el coste del terreno),
// se podría usar max(df,dc) * coste_minimo_terreno_en_camino, pero requeriría
// conocimiento global del terreno y es complicado garantizar admisibilidad.
// =============================================================================
int ComportamientoTecnico::HeuristicaTecnico(const EstadoT &actual, const EstadoT &destino) const
{
  int df = abs(actual.site.f - destino.site.f);
  int dc = abs(actual.site.c - destino.site.c);

  // Cota inferior admisible: al menos max(df,dc) movimientos WALK diagonales/ortogonales
  // y cada movimiento puede costar como mínimo 1.
  return max(df, dc);
}

// =============================================================================
// AEstrellaTecnico — A* con coste de energía real
// =============================================================================
// Implementación estándar de A* sobre el espacio de estados (f,c,brujula,zap).
//
// Cola de prioridad: min-heap ordenada por f = g + h (ComparadorNodoT).
//   Desempate primario por h, secundario por g (favorece nodos más cerca del fin).
//
// Tabla de cerrados: mejor_g[estado] = mínimo coste g conocido.
//   Esto permite el "A* graph search" que evita expandir un estado más de una vez
//   con el mismo o mayor coste, aunque el estado aparezca múltiples veces en la cola.
//
// Terminación:
//   - Si se extrae el estado destino de la cola → devolver la secuencia acumulada
//   - Si max_expansiones > 0 y se supera → corte con lista vacía
//   - Si la cola queda vacía → no hay camino, lista vacía
//
// La secuencia de acciones se copia en cada NodoT hijo (puede ser costoso en
// planes largos, pero simplifica el código y es aceptable para mapas <=128x128).
// =============================================================================
list<Action> ComportamientoTecnico::AEstrellaTecnico(const EstadoT &inicio, const EstadoT &fin, int max_expansiones)
{
  list<Action> vacio;

  priority_queue<NodoT, vector<NodoT>, ComparadorNodoT> abiertos;
  map<EstadoT, int> mejor_g; // cerrados + mejores g conocidos

  NodoT primero;
  primero.estado = inicio;
  primero.g = 0;
  primero.h = HeuristicaTecnico(inicio, fin);
  primero.f = primero.g + primero.h;

  abiertos.push(primero);
  mejor_g[inicio] = 0;
  int expansiones = 0;

  while (!abiertos.empty())
  {
    if (max_expansiones > 0 && expansiones >= max_expansiones)
      break;

    NodoT actual = abiertos.top();
    abiertos.pop();
    expansiones++;

    auto it_mejor = mejor_g.find(actual.estado);
    if (it_mejor != mejor_g.end() && actual.g > it_mejor->second)
      continue;

    if (EsDestino(actual.estado, fin))
      return actual.secuencia;

    vector<Action> acciones = {WALK, TURN_SR, TURN_SL};

    for (Action a : acciones)
    {
      if (!EsAccionAplicableTecnico(a, actual.estado))
        continue;

      EstadoT sig_estado = ApplyTecnico(a, actual.estado);
      int nuevo_g = actual.g + CosteEnergiaTecnico(a, actual.estado);

      auto it = mejor_g.find(sig_estado);
      if (it == mejor_g.end() || nuevo_g < it->second)
      {
        mejor_g[sig_estado] = nuevo_g;

        NodoT hijo;
        hijo.estado = sig_estado;
        hijo.secuencia = actual.secuencia;
        hijo.secuencia.push_back(a);
        hijo.g = nuevo_g;
        hijo.h = HeuristicaTecnico(sig_estado, fin);
        hijo.f = hijo.g + hijo.h;

        abiertos.push(hijo);
      }
    }
  }

  return vacio;
}

// =============================================================================
// NIVEL 3 — Deliberativo: A* hasta la posición de la Belkanita
// =============================================================================
// El mapa es COMPLETAMENTE CONOCIDO en N3 (sin niebla), así que A* puede
// planificar el camino óptimo desde el inicio.
//
// Flujo:
//   1. Si está sobre la Belkanita → IDLE (ya ha llegado)
//   2. Si hay choque → invalida el plan (recalcula en el siguiente turno)
//   3. Si no hay plan → lanza AEstrellaTecnico desde posición actual hasta BelPos
//   4. Ejecuta el siguiente paso del plan
//   5. Si el Ingeniero bloquea la casilla de delante → IDLE hasta que se aparte
//
// Coordinación con el Ingeniero (N3):
//   El Ingeniero en N3 se APARTA cuando detecta al Técnico cerca.
//   El Técnico simplemente espera (IDLE) si la celda frontal tiene 'i'.
// =============================================================================
Action ComportamientoTecnico::ComportamientoTecnicoNivel_3(Sensores sensores)
{
  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;

  if (sensores.reset)
  {
    hayPlan = false;
    plan.clear();
    return IDLE;
  }

  if (sensores.posF == sensores.BelPosF && sensores.posC == sensores.BelPosC)
    return IDLE;

  // Choque: esperar un turno sin recalcular (evita bucle choque->recalc->choque)
  if (sensores.choque)
  {
    hayPlan = false;
    plan.clear();
    return IDLE;
  }

  if (hayPlan && plan.empty())
    hayPlan = false;

  // Si el Ingeniero bloquea la celda de adelante, esperar (IDLE = 0 energía)
  // El Ingeniero se apartará al girar y detectar al Técnico
  if (!plan.empty() && plan.front() == WALK && sensores.agentes[2] == 'i')
    return IDLE;

  if (!hayPlan)
  {
    EstadoT inicio, fin;
    inicio.site.f = sensores.posF;
    inicio.site.c = sensores.posC;
    inicio.site.brujula = sensores.rumbo;
    inicio.zapatillas = tiene_zapatillas;

    fin.site.f = sensores.BelPosF;
    fin.site.c = sensores.BelPosC;
    fin.site.brujula = sensores.rumbo;
    fin.zapatillas = false;

    plan = AEstrellaTecnico(inicio, fin);
    VisualizaPlan(inicio.site, plan);
    hayPlan = !plan.empty();
  }

  if (hayPlan && !plan.empty())
  {
    Action accion = plan.front();
    plan.pop_front();
    return accion;
  }

  return IDLE;
}

/**
 * @brief Comportamiento del técnico para el Nivel 4.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_4(Sensores sensores)
{
  return IDLE;
}

// =============================================================================
// NIVEL 5 — Máquina de estados: instalación cooperativa con el Ingeniero
// =============================================================================
// El Técnico actúa como "instalador" dirigido por el Ingeniero.
// Protocolo de comunicación:
//   - El Ingeniero emite COME con la posición destino (sensores.GotoF, GotoC)
//   - El Técnico recibe sensores.venpaca = true y actualiza su objetivo
//   - El Técnico navega hasta el objetivo con A*
//   - Una vez en el objetivo, espera que el Ingeniero esté orientado hacia él
//   - Cuando sensores.enfrente = true → el Técnico emite INSTALL
//
// ESTADOS (EstadoTecnicoN5):
//   T5_LIBRE            → No hay objetivo asignado. IDLE o moverse por cuenta propia
//   T5_YENDO_OBJETIVO   → Navegando hacia el punto que indicó el Ingeniero con COME
//   T5_ESPERANDO_INSTALL → Está en el punto, esperando orientarse para INSTALL
//
// GESTIÓN DE COLISIONES (N5):
//   Si el Ingeniero ocupa la casilla frontal y el plan pide WALK, se espera (IDLE)
//   y se recalcula el plan (marcando la casilla del Ingeniero como 'M' temporalmente
//   para que A* la evite).
//
// IMPORTANTE: La condición sensores.enfrente = true sólo se activa cuando
//   el Técnico y el Ingeniero se miran de frente Y están adyacentes en ortogonal.
//   Por eso en T5_ESPERANDO_INSTALL el Técnico gira buscando al Ingeniero.
// =============================================================================

// =============================================================================
// FUNCIÓN: MismaCasilla
// Comprueba si una ubicación coincide con unas coordenadas (f,c).
// Ignora la orientación (brujula). Se usa en N5/N6 para detectar llegada
// al objetivo y para detectar si el Técnico ya está sobre la Belkanita.
// =============================================================================
bool ComportamientoTecnico::MismaCasilla(const ubicacion &u, int f, int c) const
{
  return u.f == f && u.c == c;
}

// =============================================================================
// FUNCIÓN: EsAdyacenteOrtogonal
// Devuelve true si dos casillas son ortogonalmente adyacentes (distancia Manhattan=1).
// Solo valida vecinos en N/S/E/O, NO diagonales.
// Uso en N5: comprobar que el Técnico y el Ingeniero están en posición de INSTALL.
// La acción INSTALL del motor exige adyacencia ortogonal — si están en diagonal
// (distancia Manhattan = 2 pero Chebyshev = 1) el INSTALL falla.
// =============================================================================
bool ComportamientoTecnico::EsAdyacenteOrtogonal(int f1, int c1, int f2, int c2) const
{
  return (abs(f1 - f2) + abs(c1 - c2)) == 1;
}

// =============================================================================
// FUNCIÓN: OrientacionHacia
// Dado que (f1,c1) y (f2,c2) son ortogonalmente adyacentes, calcula qué
// orientación cardinal debe tener el agente en (f1,c1) para mirar hacia (f2,c2).
// Solo maneja los 4 casos cardinales (norte/sur/este/oeste).
// Si se llamara con casillas diagonales, devuelve norte como valor por defecto
// (caso que no debería ocurrir si se usa correctamente junto a EsAdyacenteOrtogonal).
// Uso en N5: el Técnico calcula el giro necesario para quedar encarado al Ingeniero
// antes de emitir INSTALL.
// =============================================================================
Orientacion ComportamientoTecnico::OrientacionHacia(int f1, int c1, int f2, int c2) const
{
  if (f2 == f1 - 1 && c2 == c1)
    return norte;
  if (f2 == f1 + 1 && c2 == c1)
    return sur;
  if (f2 == f1 && c2 == c1 + 1)
    return este;
  if (f2 == f1 && c2 == c1 - 1)
    return oeste;

  return norte; // caso imposible si la relación es ortogonal
}

/**
 * @brief Comportamiento del técnico para el Nivel 5.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_5(Sensores sensores)
{
  ActualizarMapa(sensores);

  if (DEBUG_N5_TRACE_TEC)
  {
    static bool init_dbg = false;
    static EstadoTecnicoN5 ultimo_estado_dbg = T5_LIBRE;

    if (!init_dbg || ultimo_estado_dbg != estado_n5)
    {

      init_dbg = true;
      ultimo_estado_dbg = estado_n5;
    }
  }

  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;

  ubicacion actual;
  actual.f = sensores.posF;
  actual.c = sensores.posC;
  actual.brujula = sensores.rumbo;

  if (sensores.reset)
  {
    hayPlan = false;
    plan.clear();
    estado_n5 = T5_LIBRE;
    hayObjetivoN5 = false;
    espera_giro_n5 = 0;
    bloqueos_frente_n5 = 0;
    return IDLE;
  }

  // El Ingeniero emitió COME: actualizar objetivo y transicionar estado.
  // sensores.GotoF/GotoC contienen la posición del Ingeniero en el momento del COME.
  // Si ya estamos en ese punto → pasar directamente a esperar INSTALL.
  // Si no → ir hacia allí con A*.
  if (sensores.venpaca)
  {
    objetivo_n5.f = sensores.GotoF;
    objetivo_n5.c = sensores.GotoC;
    objetivo_n5.brujula = sensores.rumbo;
    hayObjetivoN5 = true;
    espera_giro_n5 = 0;
    hayPlan = false;
    plan.clear();
    bloqueos_frente_n5 = 0;

    if (MismaCasilla(actual, objetivo_n5.f, objetivo_n5.c))
      estado_n5 = T5_ESPERANDO_INSTALL;
    else
      estado_n5 = T5_YENDO_OBJETIVO;
  }

  // Choque: invalida el plan, mantiene objetivo y estado.
  if (sensores.choque)
  {
    hayPlan = false;
    plan.clear();
  }

  // Navegar hacia el objetivo marcado por el Ingeniero.
  if (estado_n5 == T5_YENDO_OBJETIVO && hayObjetivoN5)
  {
    if (MismaCasilla(actual, objetivo_n5.f, objetivo_n5.c))
    {
      hayPlan = false;
      plan.clear();
      estado_n5 = T5_ESPERANDO_INSTALL;
      espera_giro_n5 = 0;
      bloqueos_frente_n5 = 0;

      if (DEBUG_N5_TRACE_TEC)
      {
        cout << "[N5-TEC-OBJ] t=" << sensores.tiempo
             << " alcanzado=(" << objetivo_n5.f << "," << objetivo_n5.c << ")\n";
      }
      return IDLE;
    }

    if (!hayPlan)
    {
      EstadoT inicio, fin;
      inicio.site.f = sensores.posF;
      inicio.site.c = sensores.posC;
      inicio.site.brujula = sensores.rumbo;
      inicio.zapatillas = tiene_zapatillas;
      fin.site.f = objetivo_n5.f;
      fin.site.c = objetivo_n5.c;
      fin.site.brujula = sensores.rumbo;
      fin.zapatillas = false;

      int bloque_f = -1, bloque_c = -1;
      for (int idx = 1; idx <= 3; idx++)
      {
        if (sensores.agentes[idx] == 'i')
        {
          auto p = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, idx);
          bloque_f = p.first;
          bloque_c = p.second;
          break;
        }
      }

      bool bloqueo_temporal = (bloque_f >= 0 && bloque_c >= 0 &&
                               !(bloque_f == objetivo_n5.f && bloque_c == objetivo_n5.c));
      unsigned char backup_bloqueo = '?';
      if (bloqueo_temporal)
      {
        backup_bloqueo = mapaResultado[bloque_f][bloque_c];
        mapaResultado[bloque_f][bloque_c] = 'M';
      }

      int limite_astar_n6 = (sensores.nivel == 6 && mapaResultado.size() >= 70) ? 5000 : -1;
      plan = AEstrellaTecnico(inicio, fin, limite_astar_n6);

      if (bloqueo_temporal)
        mapaResultado[bloque_f][bloque_c] = backup_bloqueo;

      hayPlan = !plan.empty();
      if (hayPlan)
        VisualizaPlan(inicio.site, plan);
      else
      {
        if (sensores.nivel == 6)
          return ComportamientoTecnicoNivel_1(sensores);
        return IDLE;
      }
    }

    // Esperar si el Ingeniero bloquea la casilla frontal.
    if (!plan.empty() && plan.front() == WALK && sensores.agentes[2] == 'i')
    {
      hayPlan = false;
      plan.clear();
      bloqueos_frente_n5++;
      return IDLE;
    }

    if (!plan.empty())
    {
      Action accion = plan.front();

      // En nivel 6 el plan puede atravesar celdas aún no bien conocidas.
      // Antes de ejecutar WALK, validar con los sensores actuales para
      // evitar caídas por desnivel no previsto.
      if (accion == WALK)
      {
        char frente = ViablePorAlturaT(sensores.superficie[2],
                                       sensores.cota[2] - sensores.cota[0],
                                       tiene_zapatillas);
        if (frente == 'P' || sensores.agentes[2] == 'i')
        {
          hayPlan = false;
          plan.clear();
          if (sensores.nivel == 6)
            return ComportamientoTecnicoNivel_1(sensores);
          return IDLE;
        }
      }

      plan.pop_front();
      if (plan.empty())
        hayPlan = false;
      if (accion != TURN_SL && accion != TURN_SR)
        bloqueos_frente_n5 = 0;
      return accion;
    }

    return IDLE;
  }

  // Estado T5_ESPERANDO_INSTALL: el Técnico está en el punto objetivo.
  // Ahora busca orientarse de frente al Ingeniero para ejecutar INSTALL.
  // sensores.enfrente = true sólo cuando ambos agentes:
  //   (a) están en casillas ortogonalmente adyacentes
  //   (b) se miran directamente (rumbo Técnico apunta hacia el Ingeniero)
  if (estado_n5 == T5_ESPERANDO_INSTALL)
  {
    // Condición perfecta: ambos enfrentados → instalar y volver a libre.
    if (sensores.enfrente)
    {
      if (DEBUG_N5_TRACE_TEC)
      {
        cout << "[N5-TEC-INSTALL] t=" << sensores.tiempo
             << " pos=(" << sensores.posF << "," << sensores.posC << ")"
             << " frente=1\n";
      }
      estado_n5 = T5_LIBRE;
      espera_giro_n5 = 0;
      return INSTALL;
    }

    // Ingeniero justo enfrente pero no enfrentados aún: esperar a que gire.
    if (sensores.agentes[2] == 'i')
      return IDLE;

    // Ingeniero visible en diagonal izquierda: girar a la izquierda.
    if (sensores.agentes[1] == 'i')
      return TURN_SL;

    // Ingeniero visible en diagonal derecha: girar a la derecha.
    if (sensores.agentes[3] == 'i')
      return TURN_SR;

    return TURN_SR;
  }

  return IDLE;
}
// =============================================================================
// NIVEL 6 — Niebla de guerra + instalación cooperativa
// =============================================================================
// N6 combina exploración reactiva (niebla) con la instalación cooperativa de N5.
// El Técnico en N6 tiene DOS modos:
//
//   MODO LIBRE (estado_n5 == T5_LIBRE && !hayObjetivoN5):
//     Se mueve por cuenta propia usando greedy hacia la Belkanita (distancia
//     Manhattan). Si no puede acercarse con ninguno de los 3 movimientos
//     inmediatos, cae en ComportamientoTecnicoNivel_1 como fallback reactivo.
//     Esto sirve para explorar/descubrir el mapa mientras el Ingeniero planifica.
//
//   MODO INSTALACIÓN (venpaca==true OR estado_n5 != T5_LIBRE OR hayObjetivoN5):
//     Delega completamente en ComportamientoTecnicoNivel_5.
//     En N5 el A* tiene límite de 5000 expansiones para mapas grandes (>=70 filas).
//
//   CONDICIÓN DE TRANSICIÓN AL MODO LIBRE:
//     dist(actual, Belkanita) <= 90 — si está muy lejos, espera (IDLE).
//     Esto evita que el Técnico se aleje demasiado antes de recibir COME.
// =============================================================================
Action ComportamientoTecnico::ComportamientoTecnicoNivel_6(Sensores sensores)
{
  ActualizarMapa(sensores);

  // Si el Ingeniero envió COME o ya tenemos un objetivo activo → modo instalación
  if (sensores.venpaca || estado_n5 != T5_LIBRE || hayObjetivoN5)
    return ComportamientoTecnicoNivel_5(sensores);

  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;

  pair<int, int> pi = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 1);
  pair<int, int> pc = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 2);
  pair<int, int> pd = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 3);

  auto enRangoN6 = [&](pair<int, int> p)
  {
    return p.first >= 0 && p.first < (int)mapaResultado.size() &&
           p.second >= 0 && p.second < (int)mapaResultado[0].size();
  };

  auto viableN6 = [&](pair<int, int> p, int idx) -> bool
  {
    if (!enRangoN6(p))
      return false;
    if (sensores.agentes[idx] == 'i')
      return false;
    char v = ViablePorAlturaT(sensores.superficie[idx],
                              sensores.cota[idx] - sensores.cota[0],
                              tiene_zapatillas);
    return v != 'P';
  };

  int dist_actual = abs(sensores.BelPosF - sensores.posF) +
                    abs(sensores.BelPosC - sensores.posC);
  if (dist_actual > 90)
    return IDLE;

  Action mejor = IDLE;
  int mejor_dist = dist_actual;

  if (viableN6(pc, 2))
  {
    int d = abs(sensores.BelPosF - pc.first) + abs(sensores.BelPosC - pc.second);
    if (d < mejor_dist)
    {
      mejor_dist = d;
      mejor = WALK;
    }
  }

  if (viableN6(pi, 1))
  {
    int d = abs(sensores.BelPosF - pi.first) + abs(sensores.BelPosC - pi.second);
    if (d < mejor_dist)
    {
      mejor_dist = d;
      mejor = TURN_SL;
    }
  }

  if (viableN6(pd, 3))
  {
    int d = abs(sensores.BelPosF - pd.first) + abs(sensores.BelPosC - pd.second);
    if (d < mejor_dist)
    {
      mejor_dist = d;
      mejor = TURN_SR;
    }
  }

  if (mejor != IDLE)
    return mejor;

  return ComportamientoTecnicoNivel_1(sensores);
}

// =========================================================================
// FUNCIONES PROPORCIONADAS
// =========================================================================

// =============================================================================
// FUNCIÓN: ActualizarMapa (Técnico)
// Copia los datos de los sensores al mapaResultado (tipo superficie) y
// mapaCotas (altura) para todas las casillas visibles este turno.
// El mapa actúa como "memoria persistente" del agente: lo que se ha visto
// no se olvida aunque ya no esté en el cono de visión.
//
// Estructura del cono de visión (16 casillas):
//   [0]     = posición actual del agente
//   [1-3]   = fila a distancia 1 (izq, centro, der)
//   [4-8]   = fila a distancia 2 (5 casillas)
//   [9-15]  = fila a distancia 3 (7 casillas)
//
// El mapeo de índice → (fila,col) depende del rumbo. Para las orientaciones
// cardinales (N/S/E/O) se usa un bucle genérico. Para las diagonales
// (NE/SE/SO/NO) se listan manualmente los 15 índices porque el patrón
// diagonal no es tan regular.
//
// IMPORTANTE: llamar a ActualizarMapa CADA TURNO antes de tomar decisiones,
// para que las funciones que consultan mapaResultado/mapaCotas usen datos frescos.
// =============================================================================
void ComportamientoTecnico::ActualizarMapa(Sensores sensores)
{
  mapaResultado[sensores.posF][sensores.posC] = sensores.superficie[0];
  mapaCotas[sensores.posF][sensores.posC] = sensores.cota[0];

  int pos = 1;
  switch (sensores.rumbo)
  {
  case norte:
    for (int j = 1; j < 4; j++)
      for (int i = -j; i <= j; i++)
      {
        mapaResultado[sensores.posF - j][sensores.posC + i] = sensores.superficie[pos];
        mapaCotas[sensores.posF - j][sensores.posC + i] = sensores.cota[pos++];
      }
    break;
  case noreste:
    mapaResultado[sensores.posF - 1][sensores.posC] = sensores.superficie[1];
    mapaCotas[sensores.posF - 1][sensores.posC] = sensores.cota[1];
    mapaResultado[sensores.posF - 1][sensores.posC + 1] = sensores.superficie[2];
    mapaCotas[sensores.posF - 1][sensores.posC + 1] = sensores.cota[2];
    mapaResultado[sensores.posF][sensores.posC + 1] = sensores.superficie[3];
    mapaCotas[sensores.posF][sensores.posC + 1] = sensores.cota[3];
    mapaResultado[sensores.posF - 2][sensores.posC] = sensores.superficie[4];
    mapaCotas[sensores.posF - 2][sensores.posC] = sensores.cota[4];
    mapaResultado[sensores.posF - 2][sensores.posC + 1] = sensores.superficie[5];
    mapaCotas[sensores.posF - 2][sensores.posC + 1] = sensores.cota[5];
    mapaResultado[sensores.posF - 2][sensores.posC + 2] = sensores.superficie[6];
    mapaCotas[sensores.posF - 2][sensores.posC + 2] = sensores.cota[6];
    mapaResultado[sensores.posF - 1][sensores.posC + 2] = sensores.superficie[7];
    mapaCotas[sensores.posF - 1][sensores.posC + 2] = sensores.cota[7];
    mapaResultado[sensores.posF][sensores.posC + 2] = sensores.superficie[8];
    mapaCotas[sensores.posF][sensores.posC + 2] = sensores.cota[8];
    mapaResultado[sensores.posF - 3][sensores.posC] = sensores.superficie[9];
    mapaCotas[sensores.posF - 3][sensores.posC] = sensores.cota[9];
    mapaResultado[sensores.posF - 3][sensores.posC + 1] = sensores.superficie[10];
    mapaCotas[sensores.posF - 3][sensores.posC + 1] = sensores.cota[10];
    mapaResultado[sensores.posF - 3][sensores.posC + 2] = sensores.superficie[11];
    mapaCotas[sensores.posF - 3][sensores.posC + 2] = sensores.cota[11];
    mapaResultado[sensores.posF - 3][sensores.posC + 3] = sensores.superficie[12];
    mapaCotas[sensores.posF - 3][sensores.posC + 3] = sensores.cota[12];
    mapaResultado[sensores.posF - 2][sensores.posC + 3] = sensores.superficie[13];
    mapaCotas[sensores.posF - 2][sensores.posC + 3] = sensores.cota[13];
    mapaResultado[sensores.posF - 1][sensores.posC + 3] = sensores.superficie[14];
    mapaCotas[sensores.posF - 1][sensores.posC + 3] = sensores.cota[14];
    mapaResultado[sensores.posF][sensores.posC + 3] = sensores.superficie[15];
    mapaCotas[sensores.posF][sensores.posC + 3] = sensores.cota[15];
    break;
  case este:
    for (int j = 1; j < 4; j++)
      for (int i = -j; i <= j; i++)
      {
        mapaResultado[sensores.posF + i][sensores.posC + j] = sensores.superficie[pos];
        mapaCotas[sensores.posF + i][sensores.posC + j] = sensores.cota[pos++];
      }
    break;
  case sureste:
    mapaResultado[sensores.posF][sensores.posC + 1] = sensores.superficie[1];
    mapaCotas[sensores.posF][sensores.posC + 1] = sensores.cota[1];
    mapaResultado[sensores.posF + 1][sensores.posC + 1] = sensores.superficie[2];
    mapaCotas[sensores.posF + 1][sensores.posC + 1] = sensores.cota[2];
    mapaResultado[sensores.posF + 1][sensores.posC] = sensores.superficie[3];
    mapaCotas[sensores.posF + 1][sensores.posC] = sensores.cota[3];
    mapaResultado[sensores.posF][sensores.posC + 2] = sensores.superficie[4];
    mapaCotas[sensores.posF][sensores.posC + 2] = sensores.cota[4];
    mapaResultado[sensores.posF + 1][sensores.posC + 2] = sensores.superficie[5];
    mapaCotas[sensores.posF + 1][sensores.posC + 2] = sensores.cota[5];
    mapaResultado[sensores.posF + 2][sensores.posC + 2] = sensores.superficie[6];
    mapaCotas[sensores.posF + 2][sensores.posC + 2] = sensores.cota[6];
    mapaResultado[sensores.posF + 2][sensores.posC + 1] = sensores.superficie[7];
    mapaCotas[sensores.posF + 2][sensores.posC + 1] = sensores.cota[7];
    mapaResultado[sensores.posF + 2][sensores.posC] = sensores.superficie[8];
    mapaCotas[sensores.posF + 2][sensores.posC] = sensores.cota[8];
    mapaResultado[sensores.posF][sensores.posC + 3] = sensores.superficie[9];
    mapaCotas[sensores.posF][sensores.posC + 3] = sensores.cota[9];
    mapaResultado[sensores.posF + 1][sensores.posC + 3] = sensores.superficie[10];
    mapaCotas[sensores.posF + 1][sensores.posC + 3] = sensores.cota[10];
    mapaResultado[sensores.posF + 2][sensores.posC + 3] = sensores.superficie[11];
    mapaCotas[sensores.posF + 2][sensores.posC + 3] = sensores.cota[11];
    mapaResultado[sensores.posF + 3][sensores.posC + 3] = sensores.superficie[12];
    mapaCotas[sensores.posF + 3][sensores.posC + 3] = sensores.cota[12];
    mapaResultado[sensores.posF + 3][sensores.posC + 2] = sensores.superficie[13];
    mapaCotas[sensores.posF + 3][sensores.posC + 2] = sensores.cota[13];
    mapaResultado[sensores.posF + 3][sensores.posC + 1] = sensores.superficie[14];
    mapaCotas[sensores.posF + 3][sensores.posC + 1] = sensores.cota[14];
    mapaResultado[sensores.posF + 3][sensores.posC] = sensores.superficie[15];
    mapaCotas[sensores.posF + 3][sensores.posC] = sensores.cota[15];
    break;
  case sur:
    for (int j = 1; j < 4; j++)
      for (int i = -j; i <= j; i++)
      {
        mapaResultado[sensores.posF + j][sensores.posC - i] = sensores.superficie[pos];
        mapaCotas[sensores.posF + j][sensores.posC - i] = sensores.cota[pos++];
      }
    break;
  case suroeste:
    mapaResultado[sensores.posF + 1][sensores.posC] = sensores.superficie[1];
    mapaCotas[sensores.posF + 1][sensores.posC] = sensores.cota[1];
    mapaResultado[sensores.posF + 1][sensores.posC - 1] = sensores.superficie[2];
    mapaCotas[sensores.posF + 1][sensores.posC - 1] = sensores.cota[2];
    mapaResultado[sensores.posF][sensores.posC - 1] = sensores.superficie[3];
    mapaCotas[sensores.posF][sensores.posC - 1] = sensores.cota[3];
    mapaResultado[sensores.posF + 2][sensores.posC] = sensores.superficie[4];
    mapaCotas[sensores.posF + 2][sensores.posC] = sensores.cota[4];
    mapaResultado[sensores.posF + 2][sensores.posC - 1] = sensores.superficie[5];
    mapaCotas[sensores.posF + 2][sensores.posC - 1] = sensores.cota[5];
    mapaResultado[sensores.posF + 2][sensores.posC - 2] = sensores.superficie[6];
    mapaCotas[sensores.posF + 2][sensores.posC - 2] = sensores.cota[6];
    mapaResultado[sensores.posF + 1][sensores.posC - 2] = sensores.superficie[7];
    mapaCotas[sensores.posF + 1][sensores.posC - 2] = sensores.cota[7];
    mapaResultado[sensores.posF][sensores.posC - 2] = sensores.superficie[8];
    mapaCotas[sensores.posF][sensores.posC - 2] = sensores.cota[8];
    mapaResultado[sensores.posF + 3][sensores.posC] = sensores.superficie[9];
    mapaCotas[sensores.posF + 3][sensores.posC] = sensores.cota[9];
    mapaResultado[sensores.posF + 3][sensores.posC - 1] = sensores.superficie[10];
    mapaCotas[sensores.posF + 3][sensores.posC - 1] = sensores.cota[10];
    mapaResultado[sensores.posF + 3][sensores.posC - 2] = sensores.superficie[11];
    mapaCotas[sensores.posF + 3][sensores.posC - 2] = sensores.cota[11];
    mapaResultado[sensores.posF + 3][sensores.posC - 3] = sensores.superficie[12];
    mapaCotas[sensores.posF + 3][sensores.posC - 3] = sensores.cota[12];
    mapaResultado[sensores.posF + 2][sensores.posC - 3] = sensores.superficie[13];
    mapaCotas[sensores.posF + 2][sensores.posC - 3] = sensores.cota[13];
    mapaResultado[sensores.posF + 1][sensores.posC - 3] = sensores.superficie[14];
    mapaCotas[sensores.posF + 1][sensores.posC - 3] = sensores.cota[14];
    mapaResultado[sensores.posF][sensores.posC - 3] = sensores.superficie[15];
    mapaCotas[sensores.posF][sensores.posC - 3] = sensores.cota[15];
    break;
  case oeste:
    for (int j = 1; j < 4; j++)
      for (int i = -j; i <= j; i++)
      {
        mapaResultado[sensores.posF - i][sensores.posC - j] = sensores.superficie[pos];
        mapaCotas[sensores.posF - i][sensores.posC - j] = sensores.cota[pos++];
      }
    break;
  case noroeste:
    mapaResultado[sensores.posF][sensores.posC - 1] = sensores.superficie[1];
    mapaCotas[sensores.posF][sensores.posC - 1] = sensores.cota[1];
    mapaResultado[sensores.posF - 1][sensores.posC - 1] = sensores.superficie[2];
    mapaCotas[sensores.posF - 1][sensores.posC - 1] = sensores.cota[2];
    mapaResultado[sensores.posF - 1][sensores.posC] = sensores.superficie[3];
    mapaCotas[sensores.posF - 1][sensores.posC] = sensores.cota[3];
    mapaResultado[sensores.posF][sensores.posC - 2] = sensores.superficie[4];
    mapaCotas[sensores.posF][sensores.posC - 2] = sensores.cota[4];
    mapaResultado[sensores.posF - 1][sensores.posC - 2] = sensores.superficie[5];
    mapaCotas[sensores.posF - 1][sensores.posC - 2] = sensores.cota[5];
    mapaResultado[sensores.posF - 2][sensores.posC - 2] = sensores.superficie[6];
    mapaCotas[sensores.posF - 2][sensores.posC - 2] = sensores.cota[6];
    mapaResultado[sensores.posF - 2][sensores.posC - 1] = sensores.superficie[7];
    mapaCotas[sensores.posF - 2][sensores.posC - 1] = sensores.cota[7];
    mapaResultado[sensores.posF - 2][sensores.posC] = sensores.superficie[8];
    mapaCotas[sensores.posF - 2][sensores.posC] = sensores.cota[8];
    mapaResultado[sensores.posF][sensores.posC - 3] = sensores.superficie[9];
    mapaCotas[sensores.posF][sensores.posC - 3] = sensores.cota[9];
    mapaResultado[sensores.posF - 1][sensores.posC - 3] = sensores.superficie[10];
    mapaCotas[sensores.posF - 1][sensores.posC - 3] = sensores.cota[10];
    mapaResultado[sensores.posF - 2][sensores.posC - 3] = sensores.superficie[11];
    mapaCotas[sensores.posF - 2][sensores.posC - 3] = sensores.cota[11];
    mapaResultado[sensores.posF - 3][sensores.posC - 3] = sensores.superficie[12];
    mapaCotas[sensores.posF - 3][sensores.posC - 3] = sensores.cota[12];
    mapaResultado[sensores.posF - 3][sensores.posC - 2] = sensores.superficie[13];
    mapaCotas[sensores.posF - 3][sensores.posC - 2] = sensores.cota[13];
    mapaResultado[sensores.posF - 3][sensores.posC - 1] = sensores.superficie[14];
    mapaCotas[sensores.posF - 3][sensores.posC - 1] = sensores.cota[14];
    mapaResultado[sensores.posF - 3][sensores.posC] = sensores.superficie[15];
    mapaCotas[sensores.posF - 3][sensores.posC] = sensores.cota[15];
    break;
  }
}

// =============================================================================
// FUNCIÓN: EsCasillaTransitableLevel0
// Versión "nivel 0" de transitable: consulta el mapa de resultados y aplica
// los criterios de es_camino (C/S/D/U). El bosque ('B') y el agua ('A')
// NO son transitables aunque se tengan zapatillas — es_camino no los incluye.
// El parámetro tieneZapatillas existe por simetría con la versión del Ingeniero
// pero no se usa (el Técnico nunca cruza B en N0).
//
// Diferencia con EsCasillaTransitableTecnico (usada en A*):
//   - EsCasillaTransitableLevel0: para decisiones reactivas (N0, N1)
//   - EsCasillaTransitableTecnico: para planificación A* (N3, N5, N6)
// =============================================================================
bool ComportamientoTecnico::EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas)
{
  if (f < 0 || f >= mapaResultado.size() || c < 0 || c >= mapaResultado[0].size())
    return false;
  return es_camino(mapaResultado[f][c]); // Solo 'C', 'S', 'D', 'U' son transitables en N0
}

// =============================================================================
// FUNCIÓN: EsAccesiblePorAltura (Técnico)
// Comprueba si el desnivel entre la casilla actual y la de enfrente es <= 1.
// Obtiene la casilla frontal con Delante() y consulta mapaCotas.
// Uso en N0 y N2: decisión reactiva sin A* (el A* usa su propia comprobación
// inline en EsAccionAplicableTecnico).
// DIFERENCIA CON LA VERSIÓN DEL INGENIERO:
//   El Técnico siempre tiene límite ±1. La versión del Ingeniero acepta
//   EsAccesiblePorAltura(actual, zap) donde zap=true permite ±2.
// =============================================================================
bool ComportamientoTecnico::EsAccesiblePorAltura(const ubicacion &actual)
{
  ubicacion del = Delante(actual);
  if (del.f < 0 || del.f >= mapaCotas.size() || del.c < 0 || del.c >= mapaCotas[0].size())
    return false;
  int desnivel = abs(mapaCotas[del.f][del.c] - mapaCotas[actual.f][actual.c]);
  if (desnivel > 1)
    return false;
  return true;
}

// =============================================================================
// FUNCIÓN: Delante
// Calcula la casilla inmediatamente delante del agente según su orientación.
// Retorna una ubicacion con la misma brujula pero coordenadas desplazadas.
// Tabla de desplazamientos (fila,col):
//   norte(0)    → (-1, 0)   sur(4)     → (+1, 0)
//   noreste(1)  → (-1,+1)   suroeste(5)→ (+1,-1)
//   este(2)     → ( 0,+1)   noroeste(7)→ (-1,-1)
//   sureste(3)  → (+1,+1)   oeste(6)   → ( 0,-1)
// NO comprueba límites del mapa: el resultado puede ser fuera de rango.
// Siempre verificar antes de usar con EsAccesiblePorAltura u otras funciones.
// =============================================================================
ubicacion ComportamientoTecnico::Delante(const ubicacion &actual) const
{
  ubicacion delante = actual;
  switch (actual.brujula)
  {
  case 0:
    delante.f--;
    break; // norte
  case 1:
    delante.f--;
    delante.c++;
    break; // noreste
  case 2:
    delante.c++;
    break; // este
  case 3:
    delante.f++;
    delante.c++;
    break; // sureste
  case 4:
    delante.f++;
    break; // sur
  case 5:
    delante.f++;
    delante.c--;
    break; // suroeste
  case 6:
    delante.c--;
    break; // oeste
  case 7:
    delante.f--;
    delante.c--;
    break; // noroeste
  }
  return delante;
}

// =============================================================================
// FUNCIÓN: PintaPlan
// Imprime la secuencia de acciones de un plan por stdout para depuración.
// Formato: W(alk) J(ump) r(ight) l(eft) C(ome) I(dle) + longitud total.
// Solo para debug; no afecta al comportamiento del agente.
// =============================================================================
void ComportamientoTecnico::PintaPlan(const list<Action> &plan)
{
  auto it = plan.begin();
  while (it != plan.end())
  {
    if (*it == WALK)
    {
      cout << "W ";
    }
    else if (*it == JUMP)
    {
      cout << "J ";
    }
    else if (*it == TURN_SR)
    {
      cout << "r ";
    }
    else if (*it == TURN_SL)
    {
      cout << "l ";
    }
    else if (*it == COME)
    {
      cout << "C ";
    }
    else if (*it == IDLE)
    {
      cout << "I ";
    }
    else
    {
      cout << "-_ ";
    }
    it++;
  }
  cout << "( longitud " << plan.size() << ")" << endl;
}

// =============================================================================
// FUNCIÓN: VisualizaPlan
// Convierte el plan (lista de acciones) en una lista de casillas con su acción
// asociada (listaPlanCasillas) para que el motor las pinte en el mapa 2D.
// Simula la ejecución del plan paso a paso actualizando la orientación con
// cada giro y la posición con cada WALK/JUMP.
// Llamar después de calcular un nuevo plan para que la visualización se actualice.
// =============================================================================
void ComportamientoTecnico::VisualizaPlan(const ubicacion &st,
                                          const list<Action> &plan)
{
  listaPlanCasillas.clear();
  ubicacion cst = st;

  listaPlanCasillas.push_back({cst.f, cst.c, WALK});
  auto it = plan.begin();
  while (it != plan.end())
  {

    switch (*it)
    {
    case JUMP:
      switch (cst.brujula)
      {
      case 0:
        cst.f--;
        break;
      case 1:
        cst.f--;
        cst.c++;
        break;
      case 2:
        cst.c++;
        break;
      case 3:
        cst.f++;
        cst.c++;
        break;
      case 4:
        cst.f++;
        break;
      case 5:
        cst.f++;
        cst.c--;
        break;
      case 6:
        cst.c--;
        break;
      case 7:
        cst.f--;
        cst.c--;
        break;
      }
      if (cst.f >= 0 && cst.f < mapaResultado.size() &&
          cst.c >= 0 && cst.c < mapaResultado[0].size())
        listaPlanCasillas.push_back({cst.f, cst.c, JUMP});
      break;
    case WALK:
      switch (cst.brujula)
      {
      case 0:
        cst.f--;
        break;
      case 1:
        cst.f--;
        cst.c++;
        break;
      case 2:
        cst.c++;
        break;
      case 3:
        cst.f++;
        cst.c++;
        break;
      case 4:
        cst.f++;
        break;
      case 5:
        cst.f++;
        cst.c--;
        break;
      case 6:
        cst.c--;
        break;
      case 7:
        cst.f--;
        cst.c--;
        break;
      }
      if (cst.f >= 0 && cst.f < mapaResultado.size() &&
          cst.c >= 0 && cst.c < mapaResultado[0].size())
        listaPlanCasillas.push_back({cst.f, cst.c, WALK});
      break;
    case TURN_SR:
      cst.brujula = (Orientacion)(((int)cst.brujula + 1) % 8);
      break;
    case TURN_SL:
      cst.brujula = (Orientacion)(((int)cst.brujula + 7) % 8);
      break;
    }
    it++;
  }
}

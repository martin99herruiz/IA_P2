#include "tecnico.hpp"
#include "motorlib/util.h"
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

bool EsTransitableNivel1T(char casilla, bool zap)
{
  if (casilla == 'C' || casilla == 'S' || casilla == 'D' || casilla == 'U')
    return true;

  if (casilla == 'B' && zap)
    return true;

  return false;
}

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

char ViablePorAlturaT(char casilla, int dif, bool zap)
{
  if (abs(dif) <= 1)
    return casilla;
  else
    return 'P';
}

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

bool EsCaminoNivel0T(char x)
{
  return x == 'C' || x == 'D' || x == 'U';
}

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
    if (!(casilla == 'C' || casilla == 'D' || casilla == 'U'))
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
// Niveles del técnico

Action ComportamientoTecnico::ComportamientoTecnicoNivel_0(Sensores sensores)
{
  Action accion = IDLE;

  // =========================================================
  // 1. Fase de observación / actualización
  // =========================================================
  ActualizarMapa(sensores);
  mapaVisitas[sensores.posF][sensores.posC]++;

  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;
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
    last_action = IDLE;
    ultimaPosF = sensores.posF;
    ultimaPosC = sensores.posC;
    return IDLE;
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

/**
 * @brief Comportamiento reactivo del técnico para el Nivel 1.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
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

/**
 * @brief Comportamiento del técnico para el Nivel 2.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
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

// ============================================================
// FUNCIONES AUXILIARES NIVEL 3 - A*
// ============================================================

bool ComportamientoTecnico::EsDestino(const EstadoT &st, const EstadoT &fin) const
{
  return st.site.f == fin.site.f && st.site.c == fin.site.c;
}

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

int ComportamientoTecnico::HeuristicaTecnico(const EstadoT &actual, const EstadoT &destino) const
{
  int df = abs(actual.site.f - destino.site.f);
  int dc = abs(actual.site.c - destino.site.c);

  // Cota inferior admisible: al menos max(df,dc) movimientos WALK diagonales/ortogonales
  // y cada movimiento puede costar como mínimo 1.
  return max(df, dc);
}

list<Action> ComportamientoTecnico::AEstrellaTecnico(const EstadoT &inicio, const EstadoT &fin)
{
  list<Action> vacio;

  priority_queue<NodoT, vector<NodoT>, ComparadorNodoT> abiertos;
  map<EstadoT, int> mejor_g;

  NodoT primero;
  primero.estado = inicio;
  primero.g = 0;
  primero.h = HeuristicaTecnico(inicio, fin);
  primero.f = primero.g + primero.h;

  abiertos.push(primero);
  mejor_g[inicio] = 0;

  while (!abiertos.empty())
  {
    NodoT actual = abiertos.top();
    abiertos.pop();

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

/**
 * @brief Comportamiento del técnico para el Nivel 3.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
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

//====================================================================
//             Niveles 5  - COMPORTAMIENTO FINAL
//====================================================================

bool ComportamientoTecnico::MismaCasilla(const ubicacion &u, int f, int c) const
{
  return u.f == f && u.c == c;
}

bool ComportamientoTecnico::EsAdyacenteOrtogonal(int f1, int c1, int f2, int c2) const
{
  return (abs(f1 - f2) + abs(c1 - c2)) == 1;
}

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

  // El Ingeniero envía COME: actualizar objetivo y estado.
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

      plan = AEstrellaTecnico(inicio, fin);

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

  // Esperar posición de instalación y ejecutar INSTALL.
  if (estado_n5 == T5_ESPERANDO_INSTALL)
  {
    // Condición perfecta: emitir INSTALL y volver a esperar nuevo COME.
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
/**
 * @brief Comportamiento del técnico para el Nivel 6.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_6(Sensores sensores)
{
  ActualizarMapa(sensores);

  if (sensores.venpaca || estado_n5 != T5_LIBRE || hayObjetivoN5)
    return ComportamientoTecnicoNivel_5(sensores);

  return IDLE;
}

// =========================================================================
// FUNCIONES PROPORCIONADAS
// =========================================================================

/**
 * @brief Actualiza el mapaResultado y mapaCotas con la información de los sensores.
 * @param sensores Datos actuales de los sensores.
 */
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

/**
 * @brief Determina si una casilla es transitable para el técnico.
 * En esta práctica, si el técnico tiene zapatillas, el bosque ('B') es transitable.
 * @param f Fila de la casilla.
 * @param c Columna de la casilla.
 * @param tieneZapatillas Indica si el agente posee las zapatillas.
 * @return true si la casilla es transitable.
 */
bool ComportamientoTecnico::EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas)
{
  if (f < 0 || f >= mapaResultado.size() || c < 0 || c >= mapaResultado[0].size())
    return false;
  return es_camino(mapaResultado[f][c]); // Solo 'C', 'D', 'U' son transitables en Nivel 0
}

/**
 * @brief Comprueba si la casilla de delante es accesible por diferencia de altura.
 * Para el técnico: desnivel máximo siempre 1.
 * @param actual Estado actual del agente (fila, columna, orientacion).
 * @return true si el desnivel con la casilla de delante es admisible.
 */
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

/**
 * @brief Devuelve la posición (fila, columna) de la casilla que hay delante del agente.
 * Calcula la casilla frontal según la orientación actual (8 direcciones).
 * @param actual Estado actual del agente (fila, columna, orientacion).
 * @return Estado con la fila y columna de la casilla de enfrente.
 */
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

/**
 * @brief Imprime por consola la secuencia de acciones de un plan.
 *
 * @param plan  Lista de acciones del plan.
 */
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

/**
 * @brief Convierte un plan de acciones en una lista de casillas para
 *        su visualización en el mapa 2D.
 *
 * @param st    Estado de partida.
 * @param plan  Lista de acciones del plan.
 */
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
#include "ingeniero.hpp"

#include "motorlib/util.h"
#include <iostream>
#include <queue>
#include <set>

using namespace std;

// =========================================================================
// ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
// =========================================================================

int MejorGiroSegunMapaI(char i, char d, int vi, int vd, unsigned char mi, unsigned char md, Action last_action)
{
  bool izqTransitable = (i == 'C' || i == 'D' || i == 'U');
  bool derTransitable = (d == 'C' || d == 'D' || d == 'U');

  if (izqTransitable && !derTransitable)
    return 1;
  if (!izqTransitable && derTransitable)
    return 3;
  if (!izqTransitable && !derTransitable)
    return 0;

  // Prioridad a casillas menos visitadas
  if (vi < vd)
    return 1;
  if (vd < vi)
    return 3;

  // Empate: evitar alternancia tonta
  if (last_action == TURN_SL)
    return 1;
  if (last_action == TURN_SR)
    return 3;

  return 1;
}

pair<int, int> CoordenadaSensor123I(int f, int c, Orientacion brujula, int idx)
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

bool EsTransitableNivel1I(char casilla)
{
  return casilla == 'C' || casilla == 'S' || casilla == 'D' || casilla == 'U';
}

int PuntuarCasillaNivel1I(char visible, unsigned char conocida, int visitas, int f, int c, int ultimaF, int ultimaC)
{
  if (!EsCasilla(visible))
    return -100000;

  int score = 0;

  // PRIORIDAD MÁXIMA: descubrir mapa nuevo
  if (conocida == '?')
    score += 120;

  // PRIORIDAD ALTA: C y S (las que puntúan)
  if (visible == 'C' || visible == 'S')
    score += 50;

  // PRIORIDAD MEDIA: D y U
  if (visible == 'D' || visible == 'U')
    score += 15;

  // Penalizar visitas (más agresivo)
  score -= 5 * visitas;

  // Penalización fuerte si ya es muy repetida
  if (visitas > 5)
    score -= 20;

  // Penalizar volver atrás FUERTE
  if (f == ultimaF && c == ultimaC)
    score -= 50;

  return score;
}

int VeoCasillaInteresanteNivel1I(char i, char c, char d, unsigned char mi, unsigned char mc, unsigned char md)
{
  // Prioridad 1: descubrir terreno nuevo útil
  if ((c == 'C' || c == 'S') && mc == '?')
    return 2;
  if ((i == 'C' || i == 'S') && mi == '?')
    return 1;
  if ((d == 'C' || d == 'S') && md == '?')
    return 3;

  // Prioridad 2: seguir por C/S
  if (c == 'C' || c == 'S')
    return 2;
  if (i == 'C' || i == 'S')
    return 1;
  if (d == 'C' || d == 'S')
    return 3;

  // Prioridad 3: otras transitables
  if ((c == 'D' || c == 'U') && mc == '?')
    return 2;
  if ((i == 'D' || i == 'U') && mi == '?')
    return 1;
  if ((d == 'D' || d == 'U') && md == '?')
    return 3;

  if (c == 'D' || c == 'U')
    return 2;
  if (i == 'D' || i == 'U')
    return 1;
  if (d == 'D' || d == 'U')
    return 3;

  return 0;
}

char ViablePorAlturaI(char casilla, int dif, bool zap)
{
  if ((!zap && abs(dif) <= 1) || (zap && abs(dif) <= 2))
    return casilla;
  return 'P';
}

int VeoCasillaInteresanteI(char i, char c, char d, bool zap)
{
  if (c == 'U')
    return 2;
  if (i == 'U')
    return 1;
  if (d == 'U')
    return 3;

  if (!zap)
  {
    if (c == 'D')
      return 2;
    if (i == 'D')
      return 1;
    if (d == 'D')
      return 3;
  }

  if (c == 'C')
    return 2;
  if (i == 'C')
    return 1;
  if (d == 'C')
    return 3;

  return 0;
}

bool EsCaminoNivel0I(char x)
{
  return x == 'C' || x == 'D' || x == 'U';
}

int ElegirMovimientoNivel0I(const Sensores &sensores, char i, char c, char d, const vector<vector<int>> &mapaVisitas, int ultimaFila, int ultimaCol, bool mano_derecha)
{
  // Prioridad absoluta a U
  if (c == 'U')
    return 2;
  if (mano_derecha && d == 'U')
    return 3;
  if (!mano_derecha && i == 'U')
    return 1;
  if (d == 'U')
    return 3;
  if (i == 'U')
    return 1;

  // Prioridad a D
  if (c == 'D')
    return 2;
  if (mano_derecha && d == 'D')
    return 3;
  if (!mano_derecha && i == 'D')
    return 1;
  if (d == 'D')
    return 3;
  if (i == 'D')
    return 1;

  pair<int, int> pi = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 1);
  pair<int, int> pc = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 2);
  pair<int, int> pd = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 3);

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

    int visitas = mapaVisitas[p.first][p.second];
    int score = 0;

    // Penalización fuerte por revisitar
    score -= 60 * visitas;

    if (visitas >= 2)
      score -= 100;
    if (visitas >= 4)
      score -= 200;
    if (visitas >= 6)
      score -= 400;

    // Penalización por volver justo atrás
    if (p.first == ultimaFila && p.second == ultimaCol)
      score -= 500;

    // Bonus por seguir recto en mapas uniformes
    if (esCentro)
      score += 40;

    // Penalizar un poco las vibraciones de giro
    if (!esCentro && (sensores.rumbo % 2 != 0))
      score -= 5;

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

  // Último recurso: si el centro sigue siendo viable, usarlo
  if (scoreC > INF_NEG)
    return 2;

  return 0;
}

// Niveles iniciales (Comportamientos reactivos simples)
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_0(Sensores sensores)
{
  Action accion = IDLE;

  ActualizarMapa(sensores);
  mapaVisitas[sensores.posF][sensores.posC]++;

  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;

  if (sensores.superficie[0] == 'U')
    return IDLE;

  char i = ViablePorAlturaI(sensores.superficie[1],
                            sensores.cota[1] - sensores.cota[0],
                            tiene_zapatillas);
  char c = ViablePorAlturaI(sensores.superficie[2],
                            sensores.cota[2] - sensores.cota[0],
                            tiene_zapatillas);
  char d = ViablePorAlturaI(sensores.superficie[3],
                            sensores.cota[3] - sensores.cota[0],
                            tiene_zapatillas);

  if (sensores.agentes[1] == 't')
    i = 'P';
  if (sensores.agentes[2] == 't')
    c = 'P';
  if (sensores.agentes[3] == 't')
    d = 'P';

  bool mismaPos = (sensores.posF == ultimaPosF && sensores.posC == ultimaPosC);

  int decision = ElegirMovimientoNivel0I(sensores, i, c, d,
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
    accion = mano_derecha ? TURN_SR : TURN_SL;
    break;
  }

  // Control de bucle
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

  if (giros_consecutivos >= 7)
  {
    accion = mano_derecha ? TURN_SL : TURN_SR;
    giros_consecutivos = 0;
  }

  // Guardar casilla anterior solo si voy a avanzar
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
 * @brief Comportamiento reactivo del ingeniero para el Nivel 1.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_1(Sensores sensores)
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
      accion = TURN_SR;

    last_action = accion;
    ultimaPosF = sensores.posF;
    ultimaPosC = sensores.posC;
    turnos_sin_avanzar = 0;
    return accion;
  }

  pair<int, int> pi = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 1);
  pair<int, int> pc = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 2);
  pair<int, int> pd = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 3);

  unsigned char mi = mapaResultado[pi.first][pi.second];
  unsigned char mc = mapaResultado[pc.first][pc.second];
  unsigned char md = mapaResultado[pd.first][pd.second];

  int vi = mapaVisitas[pi.first][pi.second];
  int vc = mapaVisitas[pc.first][pc.second];
  int vd = mapaVisitas[pd.first][pd.second];

  ActualizarMapa(sensores);
  mapaVisitas[sensores.posF][sensores.posC]++;

  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;

  char i = ViablePorAlturaI(sensores.superficie[1],
                            sensores.cota[1] - sensores.cota[0],
                            tiene_zapatillas);

  char c = ViablePorAlturaI(sensores.superficie[2],
                            sensores.cota[2] - sensores.cota[0],
                            tiene_zapatillas);

  char d = ViablePorAlturaI(sensores.superficie[3],
                            sensores.cota[3] - sensores.cota[0],
                            tiene_zapatillas);
  // Evitar chocar con el Técnico
  if (sensores.agentes[1] == 't')
    i = 'P';
  if (sensores.agentes[2] == 't')
    c = 'P';
  if (sensores.agentes[3] == 't')
    d = 'P';

  int scoreI = PuntuarCasillaNivel1I(i, mi, vi, pi.first, pi.second, ultimaFila, ultimaCol);
  int scoreC = PuntuarCasillaNivel1I(c, mc, vc, pc.first, pc.second, ultimaFila, ultimaCol);
  int scoreD = PuntuarCasillaNivel1I(d, md, vd, pd.first, pd.second, ultimaFila, ultimaCol);

  if (sensores.choque)
  {
    if (scoreI > scoreD)
      accion = TURN_SL;
    else if (scoreD > scoreI)
      accion = TURN_SR;
    else if (last_action == TURN_SL || last_action == TURN_SR)
      accion = last_action; // repetir el mismo giro
    else
      accion = TURN_SR; // o TURN_SL, uno fijo por defecto
  }
  else
  {
    if (scoreC >= scoreI && scoreC >= scoreD && scoreC > -100000)
    {
      accion = WALK;
    }
    else if (scoreI > scoreD && scoreI > -100000)
    {
      accion = TURN_SL;
    }
    else if (scoreD > scoreI && scoreD > -100000)
    {
      accion = TURN_SR;
    }
    else if (scoreI > -100000) // empate izquierda/derecha
    {
      if (last_action == TURN_SL || last_action == TURN_SR)
        accion = last_action; // repetir el mismo giro
      else
        accion = TURN_SR; // o TURN_SL, uno fijo por defecto
    }
    else
    {
      if (last_action == TURN_SL || last_action == TURN_SR)
        accion = last_action; // repetir el mismo giro
      else
        accion = TURN_SR; // o TURN_SL, uno fijo por defecto
    }
  }

  // Guardamos la casilla actual antes de avanzar para penalizar volver atrás
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

//-----------------------------------------------------------------------------
//
//                            FUNCIONES 2
//
//-----------------------------------------------------------------------------

bool ComportamientoIngeniero::EnRango(int f, int c, const vector<vector<unsigned char>> &m) const
{
  return f >= 0 && f < (int)m.size() && c >= 0 && c < (int)m[0].size();
}

bool EsTransitableIngeniero(unsigned char cas)
{
  return cas == 'A' || cas == 'H' || cas == 'S' || cas == 'C' || cas == 'D' || cas == 'U';
}

ComportamientoIngeniero::EstadoI ComportamientoIngeniero::NextCasillaIngeniero(const EstadoI &st)
{
  EstadoI siguiente = st;

  switch (st.site.brujula)
  {
  case norte:
    siguiente.site.f--;
    break;
  case noreste:
    siguiente.site.f--;
    siguiente.site.c++;
    break;
  case este:
    siguiente.site.c++;
    break;
  case sureste:
    siguiente.site.f++;
    siguiente.site.c++;
    break;
  case sur:
    siguiente.site.f++;
    break;
  case suroeste:
    siguiente.site.f++;
    siguiente.site.c--;
    break;
  case oeste:
    siguiente.site.c--;
    break;
  case noroeste:
    siguiente.site.f--;
    siguiente.site.c--;
    break;
  }

  return siguiente;
}

ComportamientoIngeniero::EstadoI ComportamientoIngeniero::NextCasillaIngeniero2(const EstadoI &st)
{
  EstadoI aux = NextCasillaIngeniero(st);
  return NextCasillaIngeniero(aux);
}

bool ComportamientoIngeniero::CasillaAccesibleIngeniero(
    const EstadoI &st,
    const vector<vector<unsigned char>> &terreno,
    const vector<vector<unsigned char>> &altura)
{
  EstadoI next = NextCasillaIngeniero(st);
  if (!EnRango(next.site.f, next.site.c, terreno))
    return false;

  unsigned char cas = terreno[next.site.f][next.site.c];
  if (!EsTransitableIngeniero(cas))
    return false;

  int diff = (int)altura[next.site.f][next.site.c] - (int)altura[st.site.f][st.site.c];
  int maxDiff = st.zapatillas ? 2 : 1;

  return diff >= -maxDiff && diff <= maxDiff;
}

bool ComportamientoIngeniero::CasillaAccesibleJumpIngeniero(const EstadoI &st, const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura)
{
  EstadoI inter = NextCasillaIngeniero(st);
  EstadoI fin = NextCasillaIngeniero2(st);

  if (!EnRango(inter.site.f, inter.site.c, terreno))
    return false;
  if (!EnRango(fin.site.f, fin.site.c, terreno))
    return false;

  unsigned char casInter = terreno[inter.site.f][inter.site.c];
  unsigned char casFin = terreno[fin.site.f][fin.site.c];

  if (!EsTransitableIngeniero(casInter))
    return false;
  if (!EsTransitableIngeniero(casFin))
    return false;

  int maxDiff = st.zapatillas ? 2 : 1;

  int diffFin = abs((int)altura[fin.site.f][fin.site.c] - (int)altura[st.site.f][st.site.c]);
  if (diffFin > maxDiff)
    return false;

  return true;
}

ComportamientoIngeniero::EstadoI ComportamientoIngeniero::applyI(Action accion, const EstadoI &st, const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura)
{
  EstadoI next = st;

  switch (accion)
  {
  case WALK:
    if (CasillaAccesibleIngeniero(st, terreno, altura))
    {
      next = NextCasillaIngeniero(st);
    }
    break;

  case JUMP:
    if (CasillaAccesibleJumpIngeniero(st, terreno, altura))
    {
      next = NextCasillaIngeniero2(st);
    }
    break;

  case TURN_SR:
    next.site.brujula = (Orientacion)((next.site.brujula + 1) % 8);
    break;

  case TURN_SL:
    next.site.brujula = (Orientacion)((next.site.brujula + 7) % 8);
    break;

  default:
    break;
  }

  if (terreno[next.site.f][next.site.c] == 'D')
    next.zapatillas = true;

  return next;
}

list<Action> ComportamientoIngeniero::B_Anchura_Ingeniero(const EstadoI &inicio, const ubicacion &destino, const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura)
{
  list<Action> path;
  list<NodoI> frontier;
  set<NodoI> explored;

  NodoI current;
  current.estado = inicio;
  frontier.push_back(current);

  while (!frontier.empty())
  {
    current = frontier.front();
    frontier.pop_front();

    if (explored.find(current) != explored.end())
      continue;
    explored.insert(current);

    if (current.estado.site.f == destino.f &&
        current.estado.site.c == destino.c)
    {
      return current.secuencia;
    }

    const Action acciones[4] = {WALK, JUMP, TURN_SR, TURN_SL};

    for (Action a : acciones)
    {
      NodoI child;
      child.estado = applyI(a, current.estado, terreno, altura);

      if (child.estado == current.estado && (a == WALK || a == JUMP))
        continue;

      if (explored.find(child) == explored.end())
      {
        child.secuencia = current.secuencia;
        child.secuencia.push_back(a);
        frontier.push_back(child);
      }
    }
  }

  return path;
}

// Niveles avanzados (Uso de búsqueda)
/**
 * @brief Comportamiento del ingeniero para el Nivel 2 (búsqueda).
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_2(Sensores sensores)
{
  Action accion = IDLE;

  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;

  if (sensores.posF == sensores.BelPosF && sensores.posC == sensores.BelPosC)
  {
    hayPlan = false;
    plan.clear();
    return IDLE;
  }

  if (sensores.choque)
  {
    hayPlan = false;
    plan.clear();
  }

  if (!hayPlan)
  {
    EstadoI inicio;
    inicio.site.f = sensores.posF;
    inicio.site.c = sensores.posC;
    inicio.site.brujula = sensores.rumbo;
    inicio.zapatillas = tiene_zapatillas;

    ubicacion destino;
    destino.f = sensores.BelPosF;
    destino.c = sensores.BelPosC;

    plan = B_Anchura_Ingeniero(inicio, destino, mapaResultado, mapaCotas);
    VisualizaPlan(inicio.site, plan);
    hayPlan = !plan.empty();
  }

  if (hayPlan && !plan.empty())
  {
    accion = plan.front();
    plan.pop_front();
  }
  else
  {
    accion = IDLE;
  }

  if (plan.empty())
    hayPlan = false;

  return accion;
}

//-----------------------------------------------------------------------------
//
//                            FUNCIONES 3
//
//-----------------------------------------------------------------------------

/**
 * @brief Comportamiento del ingeniero para el Nivel 3.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_3(Sensores sensores)
{
  ubicacion actual;
  actual.f = sensores.posF;
  actual.c = sensores.posC;
  actual.brujula = sensores.rumbo;

  // Si el técnico está justo delante, girar para dejar de encararlo
  if (sensores.agentes[2] == 't')
    return TURN_SR;

  // Si el técnico está en visión cercana, intentar apartarse una sola casilla
  bool tecnico_cerca = false;
  for (int i = 1; i < 16; i++)
  {
    if (sensores.agentes[i] == 't')
    {
      tecnico_cerca = true;
      break;
    }
  }

  if (tecnico_cerca)
  {
    ubicacion d = Delante(actual);

    if (EsCasillaTransitableLevel0(d.f, d.c, tiene_zapatillas) &&
        EsAccesiblePorAltura(actual, tiene_zapatillas) &&
        mapaResultado[d.f][d.c] != 'P')
    {
      return WALK;
    }

    return TURN_SR;
  }

  return IDLE;
}

/**
 * @brief Actualiza el mapaResultado y mapaCotas con la información de los sensores.
 * @param sensores Datos actuales de los sensores.
 */
void ComportamientoIngeniero::ActualizarMapa(Sensores sensores)
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
 * @brief Determina si una casilla es transitable para el ingeniero.
 * @param f Fila de la casilla.
 * @param c Columna de la casilla.
 * @param tieneZapatillas Indica si el agente posee las zapatillas.
 * @return true si la casilla es transitable (no es muro ni precipicio).
 */
bool ComportamientoIngeniero::EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas)
{
  if (f < 0 || f >= mapaResultado.size() || c < 0 || c >= mapaResultado[0].size())
    return false;
  return es_camino(mapaResultado[f][c]); // Solo 'C', 'D', 'U' son transitables en Nivel 0
}

/**
 * @brief Comprueba si la casilla de delante es accesible por diferencia de altura.
 * Para el ingeniero: desnivel máximo 1 sin zapatillas, 2 con zapatillas.
 * @param actual Estado actual del agente (fila, columna, orientacion, zap).
 * @return true si el desnivel con la casilla de delante es admisible.
 */
bool ComportamientoIngeniero::EsAccesiblePorAltura(const ubicacion &actual, bool zap)
{
  ubicacion del = Delante(actual);
  if (del.f < 0 || del.f >= mapaCotas.size() || del.c < 0 || del.c >= mapaCotas[0].size())
    return false;
  int desnivel = abs(mapaCotas[del.f][del.c] - mapaCotas[actual.f][actual.c]);
  if (zap && desnivel > 2)
    return false;
  if (!zap && desnivel > 1)
    return false;
  return true;
}

/**
 * @brief Devuelve la posición (fila, columna) de la casilla que hay delante del agente.
 * Calcula la casilla frontal según la orientación actual (8 direcciones).
 * @param actual Estado actual del agente (fila, columna, orientacion).
 * @return Estado con la fila y columna de la casilla de enfrente.
 */
ubicacion ComportamientoIngeniero::Delante(const ubicacion &actual) const
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
void ComportamientoIngeniero::PintaPlan(const list<Action> &plan)
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
 * @brief Imprime las coordenadas y operaciones de un plan de tubería.
 *
 * @param plan  Lista de pasos (fila, columna, operación),
 *              donde operacion = -1 (DIG), operación = 1 (RAISE).
 */
void ComportamientoIngeniero::PintaPlan(const list<Paso> &plan)
{
  auto it = plan.begin();
  while (it != plan.end())
  {
    cout << it->fil << ", " << it->col << " (" << it->op << ")\n";
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
void ComportamientoIngeniero::VisualizaPlan(const ubicacion &st,
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

/**
 * @brief Convierte un plan de tubería en la lista de casillas usada
 *        por el sistema de visualización.
 *
 * @param st    Estado de partida (no utilizado directamente).
 * @param plan  Lista de pasos del plan de tubería.
 */
void ComportamientoIngeniero::VisualizaRedTuberias(const list<Paso> &plan)
{
  listaCanalizacionTuberias.clear();
  auto it = plan.begin();
  while (it != plan.end())
  {
    listaCanalizacionTuberias.push_back({it->fil, it->col, it->op});
    it++;
  }
}

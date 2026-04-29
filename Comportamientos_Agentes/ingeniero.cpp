// #include "ingeniero_0_1.cpp"
#include "ingeniero.hpp"
#include "motorlib/util.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <queue>
#include <set>
#include <map>
#include <tuple>
#include <vector>

using namespace std;

namespace
{
constexpr bool DEBUG_N5_TRACE_ING = false;
constexpr bool DEBUG_N6_TRACE_ING = false;

const char *NombreFaseN5(ComportamientoIngeniero::FaseNivel5 fase)
{
  switch (fase)
  {
  case ComportamientoIngeniero::N5_PLANIFICAR:
    return "PLANIFICAR";
  case ComportamientoIngeniero::N5_PREPARAR_TRAMO:
    return "PREPARAR_TRAMO";
  case ComportamientoIngeniero::N5_IR_A_POSICION_ING:
    return "IR_A_POSICION_ING";
  case ComportamientoIngeniero::N5_LLAMAR_TECNICO:
    return "LLAMAR_TECNICO";
  case ComportamientoIngeniero::N5_ESPERAR_TECNICO:
    return "ESPERAR_TECNICO";
  case ComportamientoIngeniero::N5_AJUSTAR_TERRENO:
    return "AJUSTAR_TERRENO";
  case ComportamientoIngeniero::N5_ALINEAR:
    return "ALINEAR";
  case ComportamientoIngeniero::N5_INSTALAR:
    return "INSTALAR";
  case ComportamientoIngeniero::N5_SIGUIENTE_TRAMO:
    return "SIGUIENTE_TRAMO";
  case ComportamientoIngeniero::N5_TERMINADO:
    return "TERMINADO";
  }
  return "?";
}

const char *NombreAccion(Action a)
{
  switch (a)
  {
  case WALK:
    return "WALK";
  case JUMP:
    return "JUMP";
  case TURN_SR:
    return "TURN_SR";
  case TURN_SL:
    return "TURN_SL";
  case DIG:
    return "DIG";
  case RAISE:
    return "RAISE";
  case COME:
    return "COME";
  case INSTALL:
    return "INSTALL";
  case IDLE:
    return "IDLE";
  default:
    return "?";
  }
}
} // namespace

// =========================================================================
// ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
// =========================================================================

/**
 * @brief Comprueba si una celda es de tipo camino transitable.
 * @param c Carácter que representa el tipo de superficie.
 * @return true si es camino ('C'), zapatillas ('D') o meta ('U').
 */
bool ComportamientoIngeniero::es_camino(unsigned char c) const
{
  return (c == 'C' || c == 'D' || c == 'U');
}

Action ComportamientoIngeniero::think(Sensores sensores)
{
  Action accion = IDLE;

  // Decisión del agente según el nivel
  switch (sensores.nivel)
  {
  case 0:
    accion = ComportamientoIngenieroNivel_0(sensores);
    break;
  case 1:
    accion = ComportamientoIngenieroNivel_1(sensores);
    break;
  case 2:
    accion = ComportamientoIngenieroNivel_2(sensores);
    break;
  case 3:
    accion = ComportamientoIngenieroNivel_3(sensores);
    break;
  case 4:
    accion = ComportamientoIngenieroNivel_4(sensores);
    break;
  case 5:
    accion = ComportamientoIngenieroNivel_5(sensores);
    break;
  case 6:
    accion = ComportamientoIngenieroNivel_6(sensores);
    break;
  }

  return accion;
}

// =========================================================================
// FUNCIONES AUXILIARES NIVELES 0
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

char ViablePorAlturaI(char casilla, int dif, bool zap)
{
  if ((!zap && abs(dif) <= 1) || (zap && abs(dif) <= 2))
    return casilla;
  return 'P';
}

Action ComportamientoIngeniero::ComportamientoIngenieroNivel_0(Sensores sensores)
{
  Action accion = IDLE;

  ActualizarMapa(sensores);
  mapaVisitas[sensores.posF][sensores.posC]++;

  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;

  bool mismaPosicion = (sensores.posF == ultimaPosF && sensores.posC == ultimaPosC);

  if (mismaPosicion && last_action != IDLE)
    turnos_sin_avanzar++;
  else
    turnos_sin_avanzar = 0;

  if ((sensores.choque && last_action == WALK) || turnos_sin_avanzar >= 6)
  {
    plan_escape = 2;
    turnos_sin_avanzar = 0;
  }

  if (plan_escape > 0)
  {
    plan_escape--;
    accion = TURN_SR;
    last_action = accion;
    ultimaPosF = sensores.posF;
    ultimaPosC = sensores.posC;
    return accion;
  }

  if (sensores.superficie[0] == 'U')
  {
    last_action = IDLE;
    ultimaPosF = sensores.posF;
    ultimaPosC = sensores.posC;
    return IDLE;
  }

  ubicacion actual;
  actual.f = sensores.posF;
  actual.c = sensores.posC;
  actual.brujula = sensores.rumbo;

  ubicacion izq = actual;
  izq.brujula = (Orientacion)((actual.brujula + 7) % 8);

  ubicacion der = actual;
  der.brujula = (Orientacion)((actual.brujula + 1) % 8);

  // Comprobar accesibilidad real por altura usando el método del profesor
  char i = 'P';
  char c = 'P';
  char d = 'P';

  ubicacion posI = Delante(izq);
  ubicacion posC = Delante(actual);
  ubicacion posD = Delante(der);

  if (EsAccesiblePorAltura(izq, tiene_zapatillas) &&
      EsCasillaTransitableLevel0(posI.f, posI.c, tiene_zapatillas))
  {
    i = sensores.superficie[1];
  }

  if (EsAccesiblePorAltura(actual, tiene_zapatillas) &&
      EsCasillaTransitableLevel0(posC.f, posC.c, tiene_zapatillas))
  {
    c = sensores.superficie[2];
  }

  if (EsAccesiblePorAltura(der, tiene_zapatillas) &&
      EsCasillaTransitableLevel0(posD.f, posD.c, tiene_zapatillas))
  {
    d = sensores.superficie[3];
  }

  // Evitar colisión con el técnico
  if (sensores.agentes[1] == 't')
    i = 'P';
  if (sensores.agentes[2] == 't')
    c = 'P';
  if (sensores.agentes[3] == 't')
    d = 'P';

  bool mismaPos = (sensores.posF == ultimaPosF && sensores.posC == ultimaPosC);

  // Salvaguarda suave contra bucles largos: solo tras MUCHAS revisitas
  // de la misma casilla, invertimos mano para probar un ciclo alternativo.
  if (mapaVisitas[sensores.posF][sensores.posC] > 35 &&
      (mapaVisitas[sensores.posF][sensores.posC] % 20) == 0)
  {
    mano_derecha = !mano_derecha;
  }

  int decision = ElegirMovimientoNivel0I(sensores, i, c, d, mapaVisitas, ultimaFila, ultimaCol, mano_derecha);

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

// =========================================================================
// FUNCIONES AUXILIARES NIVELES 1
// =========================================================================

bool EsTransitableNivel1I(char casilla)
{
  return casilla == 'C' || casilla == 'S' || casilla == 'D' || casilla == 'U';
}

int PuntuarCasillaNivel1I(char visible, unsigned char conocida, int visitas, int f, int c, int ultimaF, int ultimaC)
{
  if (!EsTransitableNivel1I(visible))
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

// =========================================================================
// FUNCIONES AUXILIARES NIVELES 2
// =========================================================================

bool ComportamientoIngeniero::EnRango(int f, int c, const vector<vector<unsigned char>> &m) const
{
  return f >= 0 && f < (int)m.size() && c >= 0 && c < (int)m[0].size();
}

bool EsTransitableIngeniero(unsigned char cas)
{
  return cas == 'A' || cas == 'H' || cas == 'S' || cas == 'C' ||
         cas == 'D' || cas == 'U' || cas == 'X';
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

bool ComportamientoIngeniero::CasillaAccesibleIngeniero(const EstadoI &st, const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura)
{
  EstadoI next = NextCasillaIngeniero(st);
  if (!EnRango(next.site.f, next.site.c, terreno))
    return false;

  unsigned char cas = terreno[next.site.f][next.site.c];
  if (!EsTransitableIngeniero(cas))
    return false;

  int diff = abs((int)altura[next.site.f][next.site.c] - (int)altura[st.site.f][st.site.c]);
  int maxDiff = st.zapatillas ? 2 : 1;

  return diff <= maxDiff;
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

list<Action> ComportamientoIngeniero::B_Anchura_Ingeniero(const EstadoI &inicio, const ubicacion &destino, const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura, int max_expansiones)
{
  list<Action> path;
  list<NodoI> frontier;
  set<NodoI> explored;
  set<EstadoI> en_frontera;

  NodoI current;
  current.estado = inicio;
  frontier.push_back(current);
  en_frontera.insert(inicio);
  int expansiones = 0;

  while (!frontier.empty() && (max_expansiones <= 0 || expansiones < max_expansiones))
  {
    current = frontier.front();
    frontier.pop_front();
    en_frontera.erase(current.estado);
    expansiones++;

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

      if (explored.find(child) == explored.end() &&
          en_frontera.find(child.estado) == en_frontera.end())
      {
        child.secuencia = current.secuencia;
        child.secuencia.push_back(a);
        frontier.push_back(child);
        en_frontera.insert(child.estado);
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

// =========================================================================
// FUNCIONES AUXILIARES NIVELES 3
// =========================================================================

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

  auto CasillaEvasionValida = [&](int f, int c)
  {
    if (f < 0 || f >= (int)mapaResultado.size() || c < 0 || c >= (int)mapaResultado[0].size())
      return false;

    unsigned char casilla = mapaResultado[f][c];
    return casilla != 'P' && casilla != 'M' && casilla != '?' && casilla != 'B';
  };

  // Si estoy exactamente sobre la Belkanita, debo salir de ahí
  if (sensores.posF == sensores.BelPosF && sensores.posC == sensores.BelPosC)
  {
    ubicacion d = Delante(actual);

    if (CasillaEvasionValida(d.f, d.c) &&
        EsAccesiblePorAltura(actual, tiene_zapatillas) &&
        mapaResultado[d.f][d.c] != 'P')
      return WALK;

    return TURN_SR;
  }

  // Si en el turno anterior decidimos apartarnos, intentamos completar ese paso
  // de escape antes de volver a quedarnos quietos.
  if (plan_escape != 0)
  {
    ubicacion d = Delante(actual);
    if (sensores.agentes[2] != 't' &&
        CasillaEvasionValida(d.f, d.c) &&
        EsAccesiblePorAltura(actual, tiene_zapatillas) &&
        mapaResultado[d.f][d.c] != 'P')
    {
      plan_escape = 0;
      return WALK;
    }

    return (plan_escape > 0) ? TURN_SR : TURN_SL;
  }

  // Si el técnico es visible en cualquier sensor, apartarse
  bool tecnicoVisible = false;
  bool tecnicoIzquierda = false;
  bool tecnicoDerecha = false;
  bool tecnicoCentro = false;
  for (int k = 1; k < 16; k++)
  {
    if (sensores.agentes[k] == 't')
    {
      tecnicoVisible = true;

      if (k == 2 || k == 6 || k == 12)
        tecnicoCentro = true;
      else if (k == 1 || k == 4 || k == 5 || k == 9 || k == 10 || k == 11)
        tecnicoIzquierda = true;
      else
        tecnicoDerecha = true;
    }
  }

  if (tecnicoVisible)
  {
    if (tecnicoDerecha && !tecnicoIzquierda && !tecnicoCentro)
    {
      plan_escape = -1;
      return TURN_SL;
    }

    plan_escape = 1;
    return TURN_SR;
  }

  return IDLE;
}

// =========================================================================
// FUNCIONES AUXILIARES NIVELES 4
// =========================================================================

bool ComportamientoIngeniero::DentroMapa(int f, int c) const
{
  return f >= 0 && f < mapaResultado.size() &&
         c >= 0 && c < mapaResultado[0].size();
}

bool ComportamientoIngeniero::CasillaValidaParaTuberia(int f, int c) const
{
  if (!DentroMapa(f, c))
    return false;

  unsigned char casilla = mapaResultado[f][c];

  // Nunca por precipicio, muro, bosque ni desconocida.
  // El Ingeniero no puede transitar por 'B', y en N5 debe poder posicionarse
  // en los extremos de los tramos para coordinar COME/INSTALL.
  if (casilla == 'P' || casilla == 'M' || casilla == '?' || casilla == 'B')
    return false;

  return true;
}

bool ComportamientoIngeniero::OperacionValidaEnCasilla(int f, int c, int op) const
{
  if (!DentroMapa(f, c))
    return false;

  unsigned char terreno = mapaResultado[f][c];
  int h = mapaCotas[f][c];

  if (op == 0)
    return true;

  // En agua no se puede modificar la altura
  if (terreno == 'A')
    return false;

  if (op == 1)
    return h < 9; // RAISE
  if (op == -1)
    return h > 1; // DIG

  return false;
}

int ComportamientoIngeniero::ImpactoInstallCasilla(int f, int c) const
{
  unsigned char t = mapaResultado[f][c];

  switch (t)
  {
  case 'A':
    return 50;
  case 'H':
    return 45;
  case 'S':
    return 25;
  case 'C':
  case 'U':
    return 15;
  default:
    return 30;
  }
}

int ComportamientoIngeniero::ImpactoOperacionCasilla(int f, int c, int op) const
{
  if (op == 0)
    return 0;

  unsigned char t = mapaResultado[f][c];

  if (op == 1) // RAISE
  {
    switch (t)
    {
    case 'H':
      return 55;
    case 'S':
      return 30;
    case 'C':
    case 'U':
      return 10;
    default:
      return 40;
    }
  }

  if (op == -1) // DIG
  {
    switch (t)
    {
    case 'H':
      return 65;
    case 'S':
      return 40;
    case 'C':
    case 'U':
      return 25;
    default:
      return 50;
    }
  }

  return 0;
}

bool ComportamientoIngeniero::YaEnPlan(const list<Paso> &plan, int f, int c) const
{
  for (const auto &p : plan)
    if (p.fil == f && p.col == c)
      return true;

  return false;
}

bool ConexionValidaPorGravedad(int h_actual, int h_siguiente)
{
  return (h_actual == h_siguiente || h_actual == h_siguiente + 1);
}

list<ComportamientoIngeniero::Paso> ComportamientoIngeniero::BuscarPlanNivel4(const ubicacion &origen, int maxImpacto, int maxNodos)
{
  list<Paso> vacio;

  struct Nodo
  {
    int f, c;
    int h;        // altura efectiva de esta casilla en el plan
    int installs; // nº de tramos añadidos después del origen
    int impacto;
    list<Paso> plan;
  };

  queue<Nodo> abiertos;
  map<tuple<int, int, int>, vector<pair<int, int>>> etiquetas;
  int mejor_installs_u = INT_MAX;
  int mejor_impacto_u = INT_MAX;
  list<Paso> mejor_plan_u;
  int nodos_expandidos = 0;

  int h0 = mapaCotas[origen.f][origen.c];

  for (int op0 = -1; op0 <= 1; ++op0)
  {
    if (!OperacionValidaEnCasilla(origen.f, origen.c, op0))
      continue;

    int hOrigen = h0 + op0;
    if (hOrigen < 0 || hOrigen > 9)
      continue;

    Nodo ini;
    ini.f = origen.f;
    ini.c = origen.c;
    ini.h = hOrigen;
    ini.installs = 0;
    ini.impacto = ImpactoOperacionCasilla(origen.f, origen.c, op0);
    ini.plan.push_back({origen.f, origen.c, op0});

    if (ini.impacto > maxImpacto)
      continue;

    abiertos.push(ini);
    etiquetas[{ini.f, ini.c, ini.h}].push_back({ini.installs, ini.impacto});
  }

  while (!abiertos.empty())
  {
    if (maxNodos > 0 && nodos_expandidos >= maxNodos)
      break;

    Nodo cur = abiertos.front();
    abiertos.pop();
    nodos_expandidos++;

    if (cur.installs > mejor_installs_u)
      break;

    if (mapaResultado[cur.f][cur.c] == 'U')
    {
      if (cur.installs < mejor_installs_u ||
          (cur.installs == mejor_installs_u && cur.impacto < mejor_impacto_u))
      {
        mejor_installs_u = cur.installs;
        mejor_impacto_u = cur.impacto;
        mejor_plan_u = cur.plan;
      }
      continue;
    }

    vector<pair<int, int>> vecinos = {
        {cur.f - 1, cur.c},
        {cur.f + 1, cur.c},
        {cur.f, cur.c - 1},
        {cur.f, cur.c + 1}};

    for (auto [nf, nc] : vecinos)
    {
      if (!DentroMapa(nf, nc))
        continue;
      if (!CasillaValidaParaTuberia(nf, nc))
        continue;

      int hbase = mapaCotas[nf][nc];

      for (int op = -1; op <= 1; ++op)
      {
        if (!OperacionValidaEnCasilla(nf, nc, op))
          continue;

        int hs = hbase + op;
        if (hs < 0 || hs > 9)
          continue;

        if (!ConexionValidaPorGravedad(cur.h, hs))
          continue;

        // El validador del motor contabiliza INSTALL en ambos extremos
        // de cada tramo (casilla previa y casilla nueva).
        int nuevoImpacto =
            cur.impacto +
            ImpactoInstallCasilla(cur.f, cur.c) +
            ImpactoInstallCasilla(nf, nc) +
            ImpactoOperacionCasilla(nf, nc, op);

        if (nuevoImpacto > maxImpacto)
          continue;

        Nodo sig = cur;
        sig.f = nf;
        sig.c = nc;
        sig.h = hs;
        sig.installs = cur.installs + 1;
        sig.impacto = nuevoImpacto;
        sig.plan.push_back({nf, nc, op});

        auto key = make_tuple(sig.f, sig.c, sig.h);
        auto &labs = etiquetas[key];

        // Dominancia: (a,b) domina a (x,y) si a<=x y b<=y.
        bool dominado = false;
        for (const auto &par : labs)
        {
          if (par.first <= sig.installs && par.second <= sig.impacto)
          {
            dominado = true;
            break;
          }
        }

        if (dominado)
          continue;

        // Eliminar etiquetas dominadas por la nueva.
        vector<pair<int, int>> filtradas;
        filtradas.reserve(labs.size() + 1);
        for (const auto &par : labs)
        {
          if (!(sig.installs <= par.first && sig.impacto <= par.second))
            filtradas.push_back(par);
        }
        filtradas.push_back({sig.installs, sig.impacto});
        labs.swap(filtradas);

        abiertos.push(sig);
      }
    }
  }

  if (!mejor_plan_u.empty())
    return mejor_plan_u;

  return vacio;
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 4.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_4(Sensores sensores)
{

  if (!planNivel4Calculado)
  {
    ubicacion origen;
    origen.f = sensores.BelPosF;
    origen.c = sensores.BelPosC;
    origen.brujula = sensores.rumbo;

    int maxImpacto = sensores.max_ecologico - sensores.ecologico;
    if (sensores.nivel == 6)
    {
      int margen_eco_n6 = max(60, maxImpacto / 7);
      maxImpacto = max(0, maxImpacto - margen_eco_n6);
    }
    if (maxImpacto < 0)
      maxImpacto = 0;

    list<Paso> plan = BuscarPlanNivel4(origen, maxImpacto);

    if (!plan.empty())
    {
      cout << "Plan de tuberias encontrado:\n";
      PintaPlan(plan);
      VisualizaRedTuberias(plan);
    }
    else
    {
      cout << "No se encontro plan valido\n";
    }

    planNivel4Calculado = true;
  }

  return IDLE;
}

// =========================================================================
// FUNCIONES AUXILIARES NIVELES 5
// =========================================================================

bool ComportamientoIngeniero::MismaCasilla(const ubicacion &u, int f, int c) const
{
  return u.f == f && u.c == c;
}

bool ComportamientoIngeniero::EsAdyacenteOrtogonal(int f1, int c1, int f2, int c2) const
{
  return (abs(f1 - f2) + abs(c1 - c2)) == 1;
}

Orientacion ComportamientoIngeniero::OrientacionHacia(int f1, int c1, int f2, int c2) const
{
  if (f2 == f1 - 1 && c2 == c1)
    return norte;
  if (f2 == f1 + 1 && c2 == c1)
    return sur;
  if (f2 == f1 && c2 == c1 + 1)
    return este;
  if (f2 == f1 && c2 == c1 - 1)
    return oeste;

  return norte; // no debería ocurrir si el tramo es ortogonal
}

bool ComportamientoIngeniero::AgenteEnObjetivo(const ubicacion &u, const ubicacion &obj) const
{
  return u.f == obj.f && u.c == obj.c;
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 5.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_5(Sensores sensores)
{
  Action accion = IDLE;

  // Aunque en N5 el mapa es conocido, las cotas sí cambian por DIG/RAISE.
  // Mantener mapaResultado/mapaCotas sincronizados evita bucles de ajuste.
  ActualizarMapa(sensores);

  if (DEBUG_N5_TRACE_ING)
  {
    static bool init_dbg = false;
    static FaseNivel5 ultima_fase_dbg = N5_PLANIFICAR;
    static int ultima_espera_dbg = -1;

    if (!init_dbg || ultima_fase_dbg != fase_n5)
    {
      cout << "[N5-ING-FASE] t=" << sensores.tiempo
           << " fase=" << NombreFaseN5(fase_n5)
           << " pos=(" << sensores.posF << "," << sensores.posC << ")"
           << " espera=" << espera_tecnico_n5 << "\n";
      init_dbg = true;
      ultima_fase_dbg = fase_n5;
      ultima_espera_dbg = espera_tecnico_n5;
    }
    else if (fase_n5 == N5_ESPERAR_TECNICO &&
             espera_tecnico_n5 > 0 &&
             (espera_tecnico_n5 % 100) == 0 &&
             espera_tecnico_n5 != ultima_espera_dbg)
    {
      cout << "[N5-ING-WAIT] t=" << sensores.tiempo
           << " pos=(" << sensores.posF << "," << sensores.posC << ")"
           << " origen=(" << paso_origen_n5.fil << "," << paso_origen_n5.col << ")"
           << " destino=(" << paso_destino_n5.fil << "," << paso_destino_n5.col << ")"
           << " enfrente=" << sensores.enfrente
           << " ag2=" << sensores.agentes[2]
           << " espera=" << espera_tecnico_n5 << "\n";
      ultima_espera_dbg = espera_tecnico_n5;
    }
  }

  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;

  ubicacion actual;
  actual.f = sensores.posF;
  actual.c = sensores.posC;
  actual.brujula = sensores.rumbo;

  auto TramoYaInstalado = [&]() -> bool
  {
    int fo = paso_origen_n5.fil;
    int co = paso_origen_n5.col;
    int fd = paso_destino_n5.fil;
    int cd = paso_destino_n5.col;

    if (fo < 0 || co < 0 || fd < 0 || cd < 0)
      return false;
    if (fo >= (int)mapaTuberias.size() || fd >= (int)mapaTuberias.size())
      return false;
    if (co >= (int)mapaTuberias[0].size() || cd >= (int)mapaTuberias[0].size())
      return false;

    unsigned char bit_o = 0;
    unsigned char bit_d = 0;

    if (fd == fo - 1 && cd == co)
    {
      bit_o = 1;
      bit_d = 16;
    } // destino al norte del origen
    else if (fd == fo + 1 && cd == co)
    {
      bit_o = 16;
      bit_d = 1;
    } // destino al sur
    else if (fd == fo && cd == co + 1)
    {
      bit_o = 4;
      bit_d = 64;
    } // destino al este
    else if (fd == fo && cd == co - 1)
    {
      bit_o = 64;
      bit_d = 4;
    } // destino al oeste
    else
    {
      return false; // no ortogonales
    }

    return ((mapaTuberias[fo][co] & bit_o) != 0) &&
           ((mapaTuberias[fd][cd] & bit_d) != 0);
  };

  switch (fase_n5)
  {
  case N5_PLANIFICAR:
  {
    ubicacion origen;
    origen.f = sensores.BelPosF;
    origen.c = sensores.BelPosC;
    origen.brujula = sensores.rumbo;

    int maxImpacto = sensores.max_ecologico - sensores.ecologico;
    if (maxImpacto < 0)
      maxImpacto = 0;
    int maxNodosN5 = -1;
    if (sensores.nivel == 6)
    {
      int margen_eco_n6 = (mapaResultado.size() >= 90) ? 60 : max(60, maxImpacto / 7);
      maxImpacto = max(0, maxImpacto - margen_eco_n6);
      maxNodosN5 = 20000;
    }
    plan_tuberias_n5 = BuscarPlanNivel4(origen, maxImpacto, maxNodosN5);

    if (plan_tuberias_n5.empty() || plan_tuberias_n5.size() < 2)
    {
      cout << "Nivel 5: no se encontro plan valido\n";
      fase_n5 = N5_TERMINADO;
      return IDLE;
    }

    cout << "Nivel 5: plan calculado\n";

    // Si el primer tramo asciende, invertimos el plan completo para recorrerlo
    // en el sentido que favorece INSTALL (Técnico en cota >= Ingeniero).
    {
      auto it0 = plan_tuberias_n5.begin();
      auto it1 = it0;
      ++it1;
      if (it1 != plan_tuberias_n5.end())
      {
        int h0 = (int)mapaCotas[it0->fil][it0->col] + it0->op;
        int h1 = (int)mapaCotas[it1->fil][it1->col] + it1->op;
        if (h0 < h1)
          plan_tuberias_n5.reverse();
      }
    }

    PintaPlan(plan_tuberias_n5);
    VisualizaRedTuberias(plan_tuberias_n5);

    if (mapaResultado.size() > 0)
      operacion_aplicada_n5.assign(mapaResultado.size(),
                                   vector<bool>(mapaResultado[0].size(), false));

    it_tramo_n5 = plan_tuberias_n5.begin();
    plan_n5_inicializado = true;
    fase_n5 = N5_PREPARAR_TRAMO;
    return IDLE;
  }

  case N5_PREPARAR_TRAMO:
  {
    auto it_sig = it_tramo_n5;
    ++it_sig;

    if (!plan_n5_inicializado || it_sig == plan_tuberias_n5.end())
    {
      cout << "Nivel 5: red instalada o sin mas tramos\n";
      fase_n5 = N5_TERMINADO;
      return IDLE;
    }

    paso_origen_n5 = *it_tramo_n5;
    paso_destino_n5 = *it_sig;

    if (sensores.nivel == 6 && mapaResultado.size() >= 90 &&
        DentroMapa(paso_origen_n5.fil, paso_origen_n5.col) &&
        DentroMapa(paso_destino_n5.fil, paso_destino_n5.col))
    {
      int h_origen = (int)mapaCotas[paso_origen_n5.fil][paso_origen_n5.col];
      int h_destino = (int)mapaCotas[paso_destino_n5.fil][paso_destino_n5.col];
      if (h_destino >= h_origen - 1 && h_destino <= h_origen)
      {
        paso_origen_n5.op = 0;
        paso_destino_n5.op = 0;
      }
      else if (paso_destino_n5.op < 0 &&
               h_destino == h_origen + 1 &&
               OperacionValidaEnCasilla(paso_origen_n5.fil, paso_origen_n5.col, 1))
      {
        paso_origen_n5.op = 1;
        paso_destino_n5.op = 0;
      }
    }

    // Para INSTALL, el motor exige h_ingeniero en [h_tecnico-1, h_tecnico].
    // Con nuestra canalización por gravedad, eso se cumple si:
    // técnico en paso_origen (más alto) e ingeniero en paso_destino (igual o más bajo).
    // 1) Ingeniero va a paso_origen
    // 2) allí hace COME para traer al Técnico a paso_origen
    // 3) Ingeniero se recoloca en paso_destino para instalar
    objetivo_ing_n5.f = paso_origen_n5.fil;
    objetivo_ing_n5.c = paso_origen_n5.col;
    objetivo_ing_n5.brujula = actual.brujula;

    objetivo_tec_n5.f = paso_origen_n5.fil;
    objetivo_tec_n5.c = paso_origen_n5.col;
    objetivo_tec_n5.brujula = actual.brujula;

    hay_plan_mov_n5 = false;
    plan_mov_n5.clear();

    tecnico_convocado_n5 = false;
    tecnico_en_posicion_n5 = false;
    tramo_instalado_n5 = false;
	    terreno_ajustado_n5 = false;
	    espera_tecnico_n5 = 0;
	    n6_rescates_mov_n5 = 0;

    fase_n5 = N5_IR_A_POSICION_ING;
    return IDLE;
  }

  case N5_IR_A_POSICION_ING:
  {
    if (AgenteEnObjetivo(actual, objetivo_ing_n5))
    {
      if (!tecnico_convocado_n5)
      {
        // Hemos llegado a la casilla desde la que debe partir el COME
        fase_n5 = N5_LLAMAR_TECNICO;
      }
      else
      {
        // Ya llamamos al técnico y nos hemos recolocado en la casilla de instalación
        fase_n5 = N5_AJUSTAR_TERRENO;
      }

      hay_plan_mov_n5 = false;
      plan_mov_n5.clear();
      return IDLE;
    }

    if (sensores.choque)
    {
      hay_plan_mov_n5 = false;
      plan_mov_n5.clear();
    }

    if (!hay_plan_mov_n5)
    {
      // En N6 solo lanzar BFS en el primer intento por segmento (n6_rescates_mov_n5==0).
      // Después usar avance_local para evitar llamar BFS caro cada tick.
      if (sensores.nivel != 6 || n6_rescates_mov_n5 == 0)
      {
        EstadoI inicio;
        inicio.site = actual;
        inicio.zapatillas = tiene_zapatillas;

        int bloque_f = -1, bloque_c = -1;
        for (int idx = 1; idx <= 3; idx++)
        {
          if (sensores.agentes[idx] == 't')
          {
            auto p = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, idx);
            bloque_f = p.first;
            bloque_c = p.second;
            break;
          }
        }

        bool bloqueo_temporal = (bloque_f >= 0 && bloque_c >= 0 &&
                                 !(bloque_f == objetivo_ing_n5.f && bloque_c == objetivo_ing_n5.c));
        unsigned char backup_bloqueo = '?';
        if (bloqueo_temporal)
        {
          backup_bloqueo = mapaResultado[bloque_f][bloque_c];
          mapaResultado[bloque_f][bloque_c] = 'M';
        }

        int bfs_limit_mov = -1;
        if (sensores.nivel == 6)
          bfs_limit_mov = (mapaResultado.size() >= 90) ? 20000 : 5000;
        plan_mov_n5 = B_Anchura_Ingeniero(inicio, objetivo_ing_n5, mapaResultado, mapaCotas, bfs_limit_mov);

        if (bloqueo_temporal)
          mapaResultado[bloque_f][bloque_c] = backup_bloqueo;

        hay_plan_mov_n5 = !plan_mov_n5.empty();

        if (DEBUG_N5_TRACE_ING)
        {
          cout << "[N5-ING-PLAN-MOV] t=" << sensores.tiempo
               << " desde=(" << actual.f << "," << actual.c << ")"
               << " hasta=(" << objetivo_ing_n5.f << "," << objetivo_ing_n5.c << ")"
               << " acciones=" << plan_mov_n5.size()
               << " zap=" << tiene_zapatillas << "\n";
        }
      }

	      if (!hay_plan_mov_n5)
	      {
	        auto avance_local_ingeniero_n6 = [&]() -> Action
	        {
	          if (sensores.nivel != 6 || n6_rescates_mov_n5 >= 300)
	            return IDLE;

	          pair<int, int> pi = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 1);
	          pair<int, int> pc = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 2);
	          pair<int, int> pd = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 3);

	          auto viable = [&](pair<int, int> p, int idx, unsigned char agente) -> bool
	          {
	            if (agente == 't')
	              return false;
	            if (!DentroMapa(p.first, p.second))
	              return false;
	            if (!EsTransitableIngeniero(sensores.superficie[idx]))
	              return false;
	            char porAltura = ViablePorAlturaI(sensores.superficie[idx],
	                                              sensores.cota[idx] - sensores.cota[0],
	                                              tiene_zapatillas);
	            return porAltura != 'P';
	          };

	          auto puntuar = [&](pair<int, int> p, int idx, unsigned char agente, bool centro) -> int
	          {
	            if (!viable(p, idx, agente))
	              return 1000000;
	            int dist = abs(objetivo_ing_n5.f - p.first) + abs(objetivo_ing_n5.c - p.second);
	            int visitas = mapaVisitas[p.first][p.second];
	            int score = dist * 20 + visitas * 4;
	            if (p.first == ultimaFila && p.second == ultimaCol)
	              score += 30;
	            if (centro)
	              score -= 5;
	            return score;
	          };

	          int dist_actual = abs(objetivo_ing_n5.f - sensores.posF) +
	                            abs(objetivo_ing_n5.c - sensores.posC);
	          int sC = puntuar(pc, 2, sensores.agentes[2], true);
	          int sI = puntuar(pi, 1, sensores.agentes[1], false);
	          int sD = puntuar(pd, 3, sensores.agentes[3], false);

	          int mejor = min(sC, min(sI, sD));
	          if (mejor >= 1000000)
	            return IDLE;

	          int mejorDist = 1000000;
	          if (mejor == sC)
	            mejorDist = abs(objetivo_ing_n5.f - pc.first) + abs(objetivo_ing_n5.c - pc.second);
	          else if (mejor == sI)
	            mejorDist = abs(objetivo_ing_n5.f - pi.first) + abs(objetivo_ing_n5.c - pi.second);
	          else
	            mejorDist = abs(objetivo_ing_n5.f - pd.first) + abs(objetivo_ing_n5.c - pd.second);

	          if (mejorDist > dist_actual + 1)
	            return IDLE;

	          n6_rescates_mov_n5++;
	          if (mejor == sC)
	            return WALK;
	          if (mejor == sI)
	            return TURN_SL;
	          return TURN_SR;
	        };

	        // Ajuste de rescate local cuando solo falta corregir desnivel con
	        // la casilla objetivo inmediata.
	        if (EsAdyacenteOrtogonal(actual.f, actual.c, objetivo_ing_n5.f, objetivo_ing_n5.c))
        {
          auto puede_rescate_n6 = [&]() -> bool
          {
            return sensores.nivel != 6 || n6_rescates_altura_n5 < 10;
          };

          int h_actual = (int)mapaCotas[actual.f][actual.c];
          int h_obj = (int)mapaCotas[objetivo_ing_n5.f][objetivo_ing_n5.c];

          if (h_actual - h_obj > 1 &&
              OperacionValidaEnCasilla(actual.f, actual.c, -1) &&
              puede_rescate_n6())
          {
            if (sensores.nivel == 6)
              n6_rescates_altura_n5++;

            if (DEBUG_N5_TRACE_ING)
            {
              cout << "[N5-ING-AJUSTE-MOV] t=" << sensores.tiempo
                   << " accion=DIG"
                   << " pos=(" << actual.f << "," << actual.c << ")"
                   << " h_act=" << h_actual
                   << " h_obj=" << h_obj
                   << " obj=(" << objetivo_ing_n5.f << "," << objetivo_ing_n5.c << ")\n";
            }
            return DIG;
          }

          if (h_obj - h_actual > 1 &&
              OperacionValidaEnCasilla(actual.f, actual.c, 1) &&
              puede_rescate_n6())
          {
            if (sensores.nivel == 6)
              n6_rescates_altura_n5++;

            if (DEBUG_N5_TRACE_ING)
            {
              cout << "[N5-ING-AJUSTE-MOV] t=" << sensores.tiempo
                   << " accion=RAISE"
                   << " pos=(" << actual.f << "," << actual.c << ")"
                   << " h_act=" << h_actual
                   << " h_obj=" << h_obj
                   << " obj=(" << objetivo_ing_n5.f << "," << objetivo_ing_n5.c << ")\n";
            }
            return RAISE;
	          }
	        }

	        Action local_n6 = avance_local_ingeniero_n6();
	        if (local_n6 != IDLE)
	          return local_n6;

	        cout << "Nivel 5: el Ingeniero no puede alcanzar su posicion objetivo\n";
	        fase_n5 = N5_TERMINADO;
        return IDLE;
      }

      VisualizaPlan(actual, plan_mov_n5);
    }

    if (!plan_mov_n5.empty() && plan_mov_n5.front() == WALK && sensores.agentes[2] == 't')
    {
      hay_plan_mov_n5 = false;
      plan_mov_n5.clear();
      return TURN_SR;
    }

    accion = plan_mov_n5.front();
    plan_mov_n5.pop_front();

    if (DEBUG_N5_TRACE_ING)
    {
      cout << "[N5-ING-MOV] t=" << sensores.tiempo
           << " pos=(" << actual.f << "," << actual.c << ")"
           << " obj=(" << objetivo_ing_n5.f << "," << objetivo_ing_n5.c << ")"
           << " accion=" << NombreAccion(accion)
           << " restante=" << plan_mov_n5.size() << "\n";
    }

    if (plan_mov_n5.empty())
      hay_plan_mov_n5 = false;

    return accion;
  }

  case N5_LLAMAR_TECNICO:
  {
    auto enRangoOp = [&](int f, int c) -> bool
    {
      return f >= 0 && !operacion_aplicada_n5.empty() &&
             f < (int)operacion_aplicada_n5.size() &&
             c >= 0 && c < (int)operacion_aplicada_n5[0].size();
    };

    // Aplicar primero la operación planificada del origen del tramo
    // (si existe y aún no se ha ejecutado).
    if (paso_origen_n5.op != 0 &&
        enRangoOp(paso_origen_n5.fil, paso_origen_n5.col) &&
        !operacion_aplicada_n5[paso_origen_n5.fil][paso_origen_n5.col] &&
        OperacionValidaEnCasilla(paso_origen_n5.fil, paso_origen_n5.col, paso_origen_n5.op))
    {
      operacion_aplicada_n5[paso_origen_n5.fil][paso_origen_n5.col] = true;
      return (paso_origen_n5.op > 0) ? RAISE : DIG;
    }

    // Ahora sí: COME envía al Técnico a la casilla actual del Ingeniero
    if (DEBUG_N5_TRACE_ING)
    {
      cout << "[N5-ING-COME] t=" << sensores.tiempo
           << " desde=(" << sensores.posF << "," << sensores.posC << ")"
           << " tec_obj=(" << paso_origen_n5.fil << "," << paso_origen_n5.col << ")"
           << " sig_ing=(" << paso_destino_n5.fil << "," << paso_destino_n5.col << ")\n";
    }

    tecnico_convocado_n5 = true;
    terreno_ajustado_n5 = false;

    // Tras COME, el Ingeniero se recoloca en paso_destino para instalar
    objetivo_ing_n5.f = paso_destino_n5.fil;
    objetivo_ing_n5.c = paso_destino_n5.col;
    objetivo_ing_n5.brujula = actual.brujula;

    fase_n5 = N5_IR_A_POSICION_ING;
    return COME;
  }

  case N5_AJUSTAR_TERRENO:
  {
    auto enRangoOp = [&](int f, int c) -> bool
    {
      return f >= 0 && !operacion_aplicada_n5.empty() &&
             f < (int)operacion_aplicada_n5.size() &&
             c >= 0 && c < (int)operacion_aplicada_n5[0].size();
    };

    // Ejecutar únicamente la operación planificada en el destino del tramo.
    if (paso_destino_n5.op != 0 &&
        enRangoOp(paso_destino_n5.fil, paso_destino_n5.col) &&
        !operacion_aplicada_n5[paso_destino_n5.fil][paso_destino_n5.col] &&
        OperacionValidaEnCasilla(paso_destino_n5.fil, paso_destino_n5.col, paso_destino_n5.op))
    {
      operacion_aplicada_n5[paso_destino_n5.fil][paso_destino_n5.col] = true;
      return (paso_destino_n5.op > 0) ? RAISE : DIG;
    }

    fase_n5 = N5_ESPERAR_TECNICO;
    return IDLE;
  }

  case N5_ESPERAR_TECNICO:
  {
    // Solo avanzamos de tramo cuando la conexión ya existe de verdad en el mapa.
    if (TramoYaInstalado())
    {
      if (DEBUG_N5_TRACE_ING)
      {
        cout << "[N5-ING-TRAMO-OK] t=" << sensores.tiempo
             << " origen=(" << paso_origen_n5.fil << "," << paso_origen_n5.col << ")"
             << " destino=(" << paso_destino_n5.fil << "," << paso_destino_n5.col << ")\n";
      }
      fase_n5 = N5_SIGUIENTE_TRAMO;
      return IDLE;
    }

    // Si ya están perfectamente enfrentados, este es el turno de instalar.
    if (sensores.enfrente)
    {
      if (DEBUG_N5_TRACE_ING)
      {
        cout << "[N5-ING-INSTALL] t=" << sensores.tiempo
             << " origen=(" << paso_origen_n5.fil << "," << paso_origen_n5.col << ")"
             << " destino=(" << paso_destino_n5.fil << "," << paso_destino_n5.col << ")\n";
      }
      return INSTALL;
    }

    // Alinear de forma determinista hacia la casilla esperada del Técnico,
    // aunque no esté en el cono frontal inmediato.
    if (EsAdyacenteOrtogonal(sensores.posF, sensores.posC,
                             paso_origen_n5.fil, paso_origen_n5.col))
    {
      Orientacion objetivo = OrientacionHacia(sensores.posF, sensores.posC,
                                              paso_origen_n5.fil, paso_origen_n5.col);
      if (sensores.rumbo != objetivo)
      {
        int diff = ((int)objetivo - (int)sensores.rumbo + 8) % 8;
        return (diff <= 4) ? TURN_SR : TURN_SL;
      }
    }

    if (espera_tecnico_n5 > 900)
    {
      fase_n5 = N5_TERMINADO;
      return IDLE;
    }

    espera_tecnico_n5++;
    return IDLE;
  }

  case N5_ALINEAR:
  {
    if (!EsAdyacenteOrtogonal(sensores.posF, sensores.posC,
                              paso_origen_n5.fil, paso_origen_n5.col))
    {
      fase_n5 = N5_ESPERAR_TECNICO;
      return IDLE;
    }

    Orientacion objetivo = OrientacionHacia(sensores.posF, sensores.posC,
                                            paso_origen_n5.fil, paso_origen_n5.col);

    if (sensores.rumbo == objetivo)
    {
      fase_n5 = N5_ESPERAR_TECNICO;
      return IDLE;
    }

    int diff = ((int)objetivo - (int)sensores.rumbo + 8) % 8;
    if (diff <= 4)
      return TURN_SR;
    else
      return TURN_SL;
  }

  case N5_SIGUIENTE_TRAMO:
  {
    ++it_tramo_n5;
    fase_n5 = N5_PREPARAR_TRAMO;
    return IDLE;
  }

  case N5_INSTALAR:
    return INSTALL;

  case N5_TERMINADO:
    return IDLE;
  }

  return accion;
}
/**
 * @brief Comportamiento del ingeniero para el Nivel 6.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_6(Sensores sensores)
{
  // Nivel 6: primero explorar (hay niebla), después reutilizar la
  // coordinación de instalación de nivel 5 cuando exista un plan válido.
  ActualizarMapa(sensores);
  if (sensores.reset)
  {
    n6_ejecutando_n5 = false;
    n6_cooldown_reintento = 0;
    n6_desconocidas_ultimo_fallo = -1;
    n6_buscando_bel = true;
    n6_pasos_busca_bel = 0;
    n6_dist_inicial_bel = -1;
	    n6_cooldown_frontier = 0;
	    n6_cooldown_plan4 = 0;
	    n6_desconocidas_ultimo_plan4 = -1;
	    n6_rescates_altura_n5 = 0;
	    n6_rescates_mov_n5 = 0;
	    hayPlan = false;
	    plan.clear();
	  }
  if (sensores.superficie[0] == 'D')
    tiene_zapatillas = true;
  if (n6_dist_inicial_bel < 0)
    n6_dist_inicial_bel = abs(sensores.BelPosF - sensores.posF) + abs(sensores.BelPosC - sensores.posC);

  auto orientacion_hacia = [&](int f1, int c1, int f2, int c2) -> Orientacion
  {
    int df = f2 - f1;
    int dc = c2 - c1;

    if (df < 0 && dc == 0)
      return norte;
    if (df < 0 && dc > 0)
      return noreste;
    if (df == 0 && dc > 0)
      return este;
    if (df > 0 && dc > 0)
      return sureste;
    if (df > 0 && dc == 0)
      return sur;
    if (df > 0 && dc < 0)
      return suroeste;
    if (df == 0 && dc < 0)
      return oeste;
    if (df < 0 && dc < 0)
      return noroeste;

    return sensores.rumbo;
  };

  auto accion_hacia_bel = [&]() -> Action
  {
    if (!n6_buscando_bel)
      return IDLE;

    if (n6_pasos_busca_bel >= 700)
      return IDLE;

    int dist_actual = abs(sensores.BelPosF - sensores.posF) + abs(sensores.BelPosC - sensores.posC);
    if (dist_actual <= 2)
      return IDLE;

    pair<int, int> pi = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 1);
    pair<int, int> pc = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 2);
    pair<int, int> pd = CoordenadaSensor123I(sensores.posF, sensores.posC, sensores.rumbo, 3);

    char vi = ViablePorAlturaI(sensores.superficie[1],
                               sensores.cota[1] - sensores.cota[0],
                               tiene_zapatillas);
    char vc = ViablePorAlturaI(sensores.superficie[2],
                               sensores.cota[2] - sensores.cota[0],
                               tiene_zapatillas);
    char vd = ViablePorAlturaI(sensores.superficie[3],
                               sensores.cota[3] - sensores.cota[0],
                               tiene_zapatillas);

    if (sensores.agentes[1] == 't')
      vi = 'P';
    if (sensores.agentes[2] == 't')
      vc = 'P';
    if (sensores.agentes[3] == 't')
      vd = 'P';

    Action mejor = IDLE;
    int mejor_dist = dist_actual;

    if (vc != 'P')
    {
      int d = abs(sensores.BelPosF - pc.first) + abs(sensores.BelPosC - pc.second);
      if (d < mejor_dist)
      {
        mejor_dist = d;
        mejor = WALK;
      }
    }

    if (vi != 'P')
    {
      int d = abs(sensores.BelPosF - pi.first) + abs(sensores.BelPosC - pi.second);
      if (d < mejor_dist)
      {
        mejor_dist = d;
        mejor = TURN_SL;
      }
    }

    if (vd != 'P')
    {
      int d = abs(sensores.BelPosF - pd.first) + abs(sensores.BelPosC - pd.second);
      if (d < mejor_dist)
      {
        mejor_dist = d;
        mejor = TURN_SR;
      }
    }

    if (mejor != IDLE)
    {
      n6_pasos_busca_bel++;
      return mejor;
    }

    Orientacion objetivo = orientacion_hacia(sensores.posF, sensores.posC,
                                             sensores.BelPosF, sensores.BelPosC);

    int diffCW = ((int)objetivo - (int)sensores.rumbo + 8) % 8;
    int diffCCW = ((int)sensores.rumbo - (int)objetivo + 8) % 8;
    if (diffCW == 0)
      return IDLE;

    n6_pasos_busca_bel++;
    return (diffCW <= diffCCW) ? TURN_SR : TURN_SL;
  };

  auto ejecutar_plan_exploracion_n6 = [&]() -> Action
  {
    if (!hayPlan || plan.empty())
    {
      hayPlan = false;
      plan.clear();
      return IDLE;
    }

    Action sig = plan.front();
    if (sig == WALK)
    {
      char frente = ViablePorAlturaI(sensores.superficie[2],
                                     sensores.cota[2] - sensores.cota[0],
                                     tiene_zapatillas);
      if (frente == 'P' || sensores.agentes[2] == 't')
      {
        hayPlan = false;
        plan.clear();
        return IDLE;
      }
    }

    plan.pop_front();
    if (plan.empty())
      hayPlan = false;
    return sig;
  };

  auto planificar_hacia_frontera_bel_n6 = [&]() -> bool
  {
    vector<tuple<int, int, int, int, int>> candidatos; // (score, distBel, distCur, f, c)
    candidatos.reserve(mapaResultado.size() * (mapaResultado.empty() ? 0 : mapaResultado[0].size()));

    const int df4[4] = {-1, 1, 0, 0};
    const int dc4[4] = {0, 0, -1, 1};

    for (int f = 0; f < (int)mapaResultado.size(); f++)
    {
      for (int c = 0; c < (int)mapaResultado[f].size(); c++)
      {
        unsigned char cas = mapaResultado[f][c];
        if (!EsTransitableIngeniero(cas))
          continue;

        bool frontera = false;
        for (int k = 0; k < 4; k++)
        {
          int nf = f + df4[k];
          int nc = c + dc4[k];
          if (!DentroMapa(nf, nc))
            continue;
          if (mapaResultado[nf][nc] == '?')
          {
            frontera = true;
            break;
          }
        }
        if (!frontera)
          continue;

        int dBel = abs(sensores.BelPosF - f) + abs(sensores.BelPosC - c);
        int dCur = abs(sensores.posF - f) + abs(sensores.posC - c);
        int score = dBel + dCur;
        candidatos.push_back(make_tuple(score, dBel, dCur, f, c));
      }
    }

    if (candidatos.empty())
      return false;

    sort(candidatos.begin(), candidatos.end());

    EstadoI inicio;
    inicio.site.f = sensores.posF;
    inicio.site.c = sensores.posC;
    inicio.site.brujula = sensores.rumbo;
    inicio.zapatillas = tiene_zapatillas;

    int intentos = 0;
    int max_intentos = min((int)candidatos.size(), 28);
    for (int i = 0; i < max_intentos; i++)
    {
      int f = get<3>(candidatos[i]);
      int c = get<4>(candidatos[i]);

      ubicacion destino;
      destino.f = f;
      destino.c = c;
      destino.brujula = sensores.rumbo;

      list<Action> nuevo_plan = B_Anchura_Ingeniero(inicio, destino, mapaResultado, mapaCotas, 4000);
      intentos++;
      if (!nuevo_plan.empty())
      {
        plan = nuevo_plan;
        hayPlan = true;
        return true;
      }
    }

    (void)intentos;
    return false;
  };

  auto explorar_n6 = [&]() -> Action
  {
    if (sensores.choque)
    {
      hayPlan = false;
      plan.clear();
    }

    Action por_plan = ejecutar_plan_exploracion_n6();
    if (por_plan != IDLE)
      return por_plan;

    if ((!hayPlan || n6_cooldown_frontier == 0) && planificar_hacia_frontera_bel_n6())
    {
      n6_cooldown_frontier = 5;
      por_plan = ejecutar_plan_exploracion_n6();
      if (por_plan != IDLE)
        return por_plan;
    }
    else if (!hayPlan || n6_cooldown_frontier == 0)
    {
      n6_cooldown_frontier = 5;
    }

    Action guiada = accion_hacia_bel();
    if (guiada != IDLE)
      return guiada;
    return ComportamientoIngenieroNivel_1(sensores);
  };

  if (n6_cooldown_reintento > 0)
    n6_cooldown_reintento--;
  if (n6_cooldown_frontier > 0)
    n6_cooldown_frontier--;
  if (n6_cooldown_plan4 > 0)
    n6_cooldown_plan4--;

  bool hayUConocida = false;
  for (int f = 0; f < (int)mapaResultado.size() && !hayUConocida; f++)
    for (int c = 0; c < (int)mapaResultado[f].size(); c++)
      if (mapaResultado[f][c] == 'U')
      {
        hayUConocida = true;
        break;
      }

  int desconocidas = 0;
  for (int f = 0; f < (int)mapaResultado.size(); f++)
    for (int c = 0; c < (int)mapaResultado[f].size(); c++)
      if (mapaResultado[f][c] == '?')
        desconocidas++;

  if (DEBUG_N6_TRACE_ING && ((int)sensores.tiempo % 100) == 0)
  {
    int distBel = abs(sensores.BelPosF - sensores.posF) + abs(sensores.BelPosC - sensores.posC);
    cout << "[N6-ING] t=" << sensores.tiempo
         << " pos=(" << sensores.posF << "," << sensores.posC << ")"
         << " bel=(" << sensores.BelPosF << "," << sensores.BelPosC << ")"
         << " dBel=" << distBel
         << " U=" << (hayUConocida ? 1 : 0)
         << " unk=" << desconocidas
         << " buscarBel=" << (n6_buscando_bel ? 1 : 0)
         << " planExp=" << (hayPlan ? 1 : 0)
         << " faseN5=" << (int)fase_n5
         << " ejecN5=" << (n6_ejecutando_n5 ? 1 : 0)
         << "\n";
  }

  if (n6_ejecutando_n5)
  {
    Action accion = ComportamientoIngenieroNivel_5(sensores);

    // Si N5 se aborta por no poder continuar, forzar vuelta a exploración
    // para descubrir más mapa antes de reintentar.
    if (fase_n5 == N5_TERMINADO)
    {
      n6_ejecutando_n5 = false;
      plan_n5_inicializado = false;
      hay_plan_mov_n5 = false;
      plan_mov_n5.clear();
      hayPlan = false;
      plan.clear();

      n6_desconocidas_ultimo_fallo = desconocidas;
      n6_cooldown_reintento = 70;
      n6_cooldown_plan4 = 20;
      n6_desconocidas_ultimo_plan4 = desconocidas;
      n6_buscando_bel = true;
      n6_pasos_busca_bel = 0;
      n6_dist_inicial_bel = -1;
      n6_rescates_altura_n5 = 0;
      n6_rescates_mov_n5 = 0;

      return ComportamientoIngenieroNivel_1(sensores);
    }

    return accion;
  }

  if (!hayUConocida)
    return explorar_n6();

  // Si acabamos de fallar en N5, explorar un poco antes de reintentar.
  if (n6_cooldown_reintento > 0)
    return explorar_n6();

  if (n6_cooldown_plan4 > 0)
    return explorar_n6();

  ubicacion origen;
  origen.f = sensores.BelPosF;
  origen.c = sensores.BelPosC;
  origen.brujula = sensores.rumbo;

  int maxImpacto = sensores.max_ecologico - sensores.ecologico;
  if (sensores.nivel == 6)
  {
    int margen_eco_n6 = (mapaResultado.size() >= 90) ? 60 : max(60, maxImpacto / 7);
    maxImpacto = max(0, maxImpacto - margen_eco_n6);
  }
  if (maxImpacto < 0)
    maxImpacto = 0;

  list<Paso> candidato = BuscarPlanNivel4(origen, maxImpacto, 20000);
  n6_desconocidas_ultimo_plan4 = desconocidas;

  // Si ya hay suficiente mapa conocido para planificar tuberías, activar N5.
  if (!candidato.empty() && candidato.size() >= 2)
  {
    fase_n5 = N5_PLANIFICAR;
    plan_n5_inicializado = false;
    hay_plan_mov_n5 = false;
    plan_mov_n5.clear();
    n6_ejecutando_n5 = true;
    n6_desconocidas_ultimo_fallo = -1;
    n6_buscando_bel = false;
    n6_pasos_busca_bel = 0;
    n6_dist_inicial_bel = -1;
    n6_cooldown_plan4 = 0;
    n6_desconocidas_ultimo_plan4 = -1;
    n6_rescates_altura_n5 = 0;
    n6_rescates_mov_n5 = 0;
    hayPlan = false;
    plan.clear();

    return ComportamientoIngenieroNivel_5(sensores);
  }

  // Aún no hay plan: explorar para descubrir terreno/cotas/U.
  n6_cooldown_plan4 = 25;
  return explorar_n6();
}

// =========================================================================
// FUNCIONES PROPORCIONADAS
// =========================================================================

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
void ComportamientoIngeniero::PintaPlan(const list<ComportamientoIngeniero::Paso> &plan)
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
 * @brief Convierte un plan de tubería en la lista de casillas usada por el sistema de visualización.
 * @param st    Estado de partida (no utilizado directamente).
 * @param plan  Lista de pasos del plan de tubería.
 */
void ComportamientoIngeniero::VisualizaRedTuberias(const list<ComportamientoIngeniero::Paso> &plan)
{
  listaCanalizacionTuberias.clear();
  auto it = plan.begin();
  while (it != plan.end())
  {
    listaCanalizacionTuberias.push_back({it->fil, it->col, it->op});
    it++;
  }
}

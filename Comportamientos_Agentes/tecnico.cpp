#include "tecnico.hpp"
#include "motorlib/util.h"
#include <iostream>
#include <queue>
#include <set>

using namespace std;

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

int ElegirMovimientoNivel0T(const Sensores &sensores,
                            char i, char c, char d,
                            const vector<vector<int>> &mapaVisitas,
                            int ultimaFila, int ultimaCol,
                            bool mano_derecha)
{
  pair<int, int> pi = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 1);
  pair<int, int> pc = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 2);
  pair<int, int> pd = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 3);

  auto enRango = [&](pair<int, int> p)
  {
    return p.first >= 0 && p.first < (int)mapaVisitas.size() &&
           p.second >= 0 && p.second < (int)mapaVisitas[0].size();
  };

  auto visitas = [&](pair<int, int> p)
  {
    if (!enRango(p))
      return 999999;
    return mapaVisitas[p.first][p.second];
  };

  auto penalizaAtras = [&](pair<int, int> p)
  {
    return (p.first == ultimaFila && p.second == ultimaCol);
  };

  // Prioridad absoluta a U
  if (c == 'U')
    return 2;
  if (!mano_derecha && i == 'U')
    return 1;
  if (mano_derecha && d == 'U')
    return 3;
  if (i == 'U')
    return 1;
  if (d == 'U')
    return 3;

  // Entre caminos, elegir el menos visitado
  vector<pair<int, int>> opcionesC;
  if (i == 'C')
    opcionesC.push_back({1, visitas(pi) + (penalizaAtras(pi) ? 100 : 0)});
  if (c == 'C')
    opcionesC.push_back({2, visitas(pc) + (penalizaAtras(pc) ? 100 : 0)});
  if (d == 'C')
    opcionesC.push_back({3, visitas(pd) + (penalizaAtras(pd) ? 100 : 0)});

  if (!opcionesC.empty())
  {
    int mejorDir = opcionesC[0].first;
    int mejorCoste = opcionesC[0].second;

    for (int k = 1; k < (int)opcionesC.size(); k++)
    {
      if (opcionesC[k].second < mejorCoste)
      {
        mejorCoste = opcionesC[k].second;
        mejorDir = opcionesC[k].first;
      }
      else if (opcionesC[k].second == mejorCoste)
      {
        // desempate: favorecer seguir recto
        if (opcionesC[k].first == 2)
          mejorDir = 2;
        else if (mejorDir != 2)
        {
          if (mano_derecha && opcionesC[k].first == 3)
            mejorDir = 3;
          if (!mano_derecha && opcionesC[k].first == 1)
            mejorDir = 1;
        }
      }
    }
    return mejorDir;
  }

  // Luego zapatillas
  vector<pair<int, int>> opcionesD;
  if (i == 'D')
    opcionesD.push_back({1, visitas(pi) + (penalizaAtras(pi) ? 100 : 0)});
  if (c == 'D')
    opcionesD.push_back({2, visitas(pc) + (penalizaAtras(pc) ? 100 : 0)});
  if (d == 'D')
    opcionesD.push_back({3, visitas(pd) + (penalizaAtras(pd) ? 100 : 0)});

  if (!opcionesD.empty())
  {
    int mejorDir = opcionesD[0].first;
    int mejorCoste = opcionesD[0].second;

    for (int k = 1; k < (int)opcionesD.size(); k++)
    {
      if (opcionesD[k].second < mejorCoste)
      {
        mejorCoste = opcionesD[k].second;
        mejorDir = opcionesD[k].first;
      }
      else if (opcionesD[k].second == mejorCoste)
      {
        if (opcionesD[k].first == 2)
          mejorDir = 2;
        else if (mejorDir != 2)
        {
          if (mano_derecha && opcionesD[k].first == 3)
            mejorDir = 3;
          if (!mano_derecha && opcionesD[k].first == 1)
            mejorDir = 1;
        }
      }
    }
    return mejorDir;
  }

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

  // Si ya estoy en una planta, objetivo cumplido
  if (sensores.superficie[0] == 'U')
  {
    last_action = IDLE;
    ultimaPosF = sensores.posF;
    ultimaPosC = sensores.posC;
    return IDLE;
  }

  // =========================================================
  // 2. Construcción de las 3 opciones inmediatas (izq-centro-der)
  //    usando más los métodos del profesor
  // =========================================================
  pair<int,int> pi = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 1);
  pair<int,int> pc = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 2);
  pair<int,int> pd = CoordenadaSensor123T(sensores.posF, sensores.posC, sensores.rumbo, 3);

  auto enRangoMapa = [&](pair<int,int> p) {
    return p.first >= 0 && p.first < (int)mapaResultado.size() &&
           p.second >= 0 && p.second < (int)mapaResultado[0].size();
  };

  auto viableLateral = [&](pair<int,int> p, char casillaSensor, int idxSensor) {
    if (!enRangoMapa(p)) return 'P';

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

  // Casillas ocupadas por el Ingeniero = no disponibles
  if (sensores.agentes[1] == 'i') i = 'P';
  if (sensores.agentes[2] == 'i') c = 'P';
  if (sensores.agentes[3] == 'i') d = 'P';

  // =========================================================
  // 3. Reglas reactivas de conflicto
  // =========================================================

  // Cesión mínima al Ingeniero: si está justo delante, no compito
  if (sensores.agentes[2] == 'i')
  {
    accion = mano_derecha ? TURN_SR : TURN_SL;
    last_action = accion;
    ultimaPosF = sensores.posF;
    ultimaPosC = sensores.posC;
    return accion;
  }

  // Si choqué intentando avanzar, no insisto
  if (sensores.choque && last_action == WALK)
  {
    accion = mano_derecha ? TURN_SL : TURN_SR;
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
  return (c == 'C' || c == 'D' || c == 'U');
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
  return IDLE;
}

/**
 * @brief Comportamiento del técnico para el Nivel 3.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_3(Sensores sensores)
{
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

/**
 * @brief Comportamiento del técnico para el Nivel 5.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_5(Sensores sensores)
{
  return IDLE;
}

/**
 * @brief Comportamiento del técnico para el Nivel 6.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_6(Sensores sensores)
{
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
  return es_camino(mapaResultado[f][c]); // Solo 'C', 'S', 'D', 'U' son transitables en Nivel 0
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

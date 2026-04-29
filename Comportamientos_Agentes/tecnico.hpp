#ifndef COMPORTAMIENTOTECNICO_H
#define COMPORTAMIENTOTECNICO_H

#include <chrono>
#include <cmath>
#include <list>
#include <queue>
#include <set>
#include <thread>
#include <time.h>
#include <vector>

#include "comportamientos/comportamiento.hpp"

class ComportamientoTecnico : public Comportamiento
{
public:
  ComportamientoTecnico(unsigned int size = 0) : Comportamiento(size)
  {
    last_action = IDLE;
    tiene_zapatillas = false;
    ultimaFila = -1;
    ultimaCol = -1;
    ultimaPosF = -1;
    ultimaPosC = -1;
    turnos_sin_avanzar = 0;
    mapaVisitas = std::vector<std::vector<int>>(size, std::vector<int>(size, 0));
    mano_derecha = false;
    plan_escape = 0;
    giros_consecutivos = 0;

    hayPlan = false;

    estado_n5 = T5_LIBRE;
    hayObjetivoN5 = false;
    instalar_este_turno_n5 = false;
    espera_giro_n5 = 0;
    bloqueos_frente_n5 = 0;
  }

  ComportamientoTecnico(std::vector<std::vector<unsigned char>> mapaR,
                        std::vector<std::vector<unsigned char>> mapaC)
      : Comportamiento(mapaR, mapaC)
  {
    last_action = IDLE;
    tiene_zapatillas = false;
    ultimaFila = -1;
    ultimaCol = -1;
    ultimaPosF = -1;
    ultimaPosC = -1;
    turnos_sin_avanzar = 0;
    mapaVisitas = vector<vector<int>>(mapaR.size(), vector<int>(mapaR[0].size(), 0));
    mano_derecha = false;
    plan_escape = 0;
    giros_consecutivos = 0;

    hayPlan = false;

    estado_n5 = T5_LIBRE;
    hayObjetivoN5 = false;
    instalar_este_turno_n5 = false;
    espera_giro_n5 = 0;
    bloqueos_frente_n5 = 0;
  }

  ComportamientoTecnico(const ComportamientoTecnico &comport) : Comportamiento(comport) {}
  ~ComportamientoTecnico() {}

  Action think(Sensores sensores);

  ComportamientoTecnico *clone()
  {
    return new ComportamientoTecnico(*this);
  }

  Action ComportamientoTecnicoNivel_0(Sensores sensores);
  Action ComportamientoTecnicoNivel_1(Sensores sensores);
  Action ComportamientoTecnicoNivel_2(Sensores sensores);
  Action ComportamientoTecnicoNivel_3(Sensores sensores);
  Action ComportamientoTecnicoNivel_4(Sensores sensores);
  Action ComportamientoTecnicoNivel_5(Sensores sensores);
  Action ComportamientoTecnicoNivel_6(Sensores sensores);

protected:
  void ActualizarMapa(Sensores sensores);
  bool EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas);
  bool EsAccesiblePorAltura(const ubicacion &actual);
  ubicacion Delante(const ubicacion &actual) const;
  bool es_camino(unsigned char c) const;
  void PintaPlan(const std::list<Action> &plan);
  void VisualizaPlan(const ubicacion &st, const std::list<Action> &plan);

private:
  // =========================================================
  // ESTRUCTURAS PARA BÚSQUEDA DELIBERATIVA NIVEL 3
  // =========================================================
  struct EstadoT
  {
    ubicacion site;
    bool zapatillas = false;

    bool operator==(const EstadoT &other) const
    {
      return site.f == other.site.f &&
             site.c == other.site.c &&
             site.brujula == other.site.brujula &&
             zapatillas == other.zapatillas;
    }

    bool operator<(const EstadoT &other) const
    {
      if (site.f != other.site.f)
        return site.f < other.site.f;
      if (site.c != other.site.c)
        return site.c < other.site.c;
      if (site.brujula != other.site.brujula)
        return site.brujula < other.site.brujula;
      return zapatillas < other.zapatillas;
    }
  };

  struct NodoT
  {
    EstadoT estado;
    std::list<Action> secuencia;
    int g; // coste acumulado de energía
    int h; // heurística
    int f; // g + h
  };

  struct ComparadorNodoT
  {
    bool operator()(const NodoT &a, const NodoT &b) const
    {
      if (a.f != b.f)
        return a.f > b.f; // priority_queue mínima
      if (a.h != b.h)
        return a.h > b.h; // desempate
      return a.g > b.g;
    }
  };

  // =========================================================
  // ESTADO INTERNO NIVEL 5
  // =========================================================
  enum EstadoTecnicoN5
  {
    T5_LIBRE,
    T5_YENDO_OBJETIVO,
    T5_ESPERANDO_INSTALL
  };

  // =========================================================
  // FUNCIONES AUXILIARES NIVEL 3
  // =========================================================
  EstadoT NextCasillaTecnico(const EstadoT &st) const;
  EstadoT ApplyTecnico(Action accion, const EstadoT &st) const;

  bool EsCasillaTransitableTecnico(int f, int c, bool zapatillas) const;
  bool EsAccionAplicableTecnico(Action accion, const EstadoT &st) const;

  int CosteEnergiaTecnico(Action accion, const EstadoT &st) const;
  int HeuristicaTecnico(const EstadoT &actual, const EstadoT &destino) const;

  std::list<Action> AEstrellaTecnico(const EstadoT &inicio, const EstadoT &fin, int max_expansiones = -1);
  bool EsDestino(const EstadoT &st, const EstadoT &fin) const;

  // =========================================================
  // FUNCIONES AUXILIARES NIVEL 5
  // =========================================================
  bool MismaCasilla(const ubicacion &u, int f, int c) const;
  Orientacion OrientacionHacia(int f1, int c1, int f2, int c2) const;
  bool EsAdyacenteOrtogonal(int f1, int c1, int f2, int c2) const;

  // =========================================================
  // VARIABLES DE ESTADO
  // =========================================================
  Action last_action;
  bool tiene_zapatillas;
  std::vector<std::vector<int>> mapaVisitas;
  int ultimaFila, ultimaCol;
  int ultimaPosF, ultimaPosC;
  int turnos_sin_avanzar;
  int giros_consecutivos;
  int plan_escape;
  bool mano_derecha;

  // Deliberativo
  bool hayPlan;
  std::list<Action> plan;

  // Nivel 5
  EstadoTecnicoN5 estado_n5;
  ubicacion objetivo_n5;
  bool hayObjetivoN5;
  bool instalar_este_turno_n5;
  int espera_giro_n5;
  int bloqueos_frente_n5;
};

#endif

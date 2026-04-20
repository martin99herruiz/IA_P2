#ifndef COMPORTAMIENTOTECNICO_H
#define COMPORTAMIENTOTECNICO_H

#include <chrono>
#include <time.h>
#include <thread>
#include <list>
#include <queue>
#include <set>
#include <vector>
#include <cmath>

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
    mapaVisitas = vector<vector<int>>(size, vector<int>(size, 0));
    mano_derecha = false;
    plan_escape = 0;
    giros_consecutivos = 0;

    hayPlan = false;
  }

  ComportamientoTecnico(std::vector<std::vector<unsigned char>> mapaR,
                        std::vector<std::vector<unsigned char>> mapaC) : Comportamiento(mapaR, mapaC)
  {
    last_action = IDLE;
    tiene_zapatillas = false;
    ultimaFila = -1;
    ultimaCol = -1;
    ultimaPosF = -1;
    ultimaPosC = -1;
    turnos_sin_avanzar = 0;
    mapaVisitas = vector<vector<int>>(mapaR.size(), vector<int>(mapaR.size(), 0));
    mano_derecha = false;
    plan_escape = 0;
    giros_consecutivos = 0;

    hayPlan = false;
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
  void PintaPlan(const list<Action> &plan);
  void PintaPlan(const list<Paso> &plan);
  void VisualizaPlan(const ubicacion &st, const list<Action> &plan);

private:
  // =========================================================
  // ESTRUCTURAS PARA BÚSQUEDA DELIBERATIVA NIVEL 3
  // =========================================================
  struct EstadoT
  {
    ubicacion site;
    bool zapatillas;

    bool operator==(const EstadoT &other) const
    {
      return site.f == other.site.f &&
             site.c == other.site.c &&
             site.brujula == other.site.brujula &&
             zapatillas == other.zapatillas;
    }

    bool operator<(const EstadoT &other) const
    {
      if (site.f != other.site.f) return site.f < other.site.f;
      if (site.c != other.site.c) return site.c < other.site.c;
      if (site.brujula != other.site.brujula) return site.brujula < other.site.brujula;
      return zapatillas < other.zapatillas;
    }
  };

  struct NodoT
  {
    EstadoT estado;
    list<Action> secuencia;
    int g; // coste acumulado de energía
    int h; // heurística
    int f; // g + h
  };

  struct ComparadorNodoT
  {
    bool operator()(const NodoT &a, const NodoT &b) const
    {
      if (a.f != b.f) return a.f > b.f;   // priority_queue mínima
      if (a.h != b.h) return a.h > b.h;   // desempate
      return a.g > b.g;
    }
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

  list<Action> AEstrellaTecnico(const EstadoT &inicio, const EstadoT &fin);
  bool EsDestino(const EstadoT &st, const EstadoT &fin) const;

  // =========================================================
  // VARIABLES DE ESTADO
  // =========================================================
  Action last_action;
  bool tiene_zapatillas;
  vector<vector<int>> mapaVisitas;
  int ultimaFila, ultimaCol;
  int ultimaPosF, ultimaPosC;
  int turnos_sin_avanzar;
  int giros_consecutivos;
  int plan_escape;
  bool mano_derecha;

  // Deliberativo
  bool hayPlan;
  list<Action> plan;
};

#endif
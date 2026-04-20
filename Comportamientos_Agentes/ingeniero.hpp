#ifndef COMPORTAMIENTOINGENIERO_H
#define COMPORTAMIENTOINGENIERO_H

#include <chrono>
#include <list>
#include <map>
#include <set>
#include <thread>
#include <time.h>
#include <vector>

#include "comportamientos/comportamiento.hpp"

class ComportamientoIngeniero : public Comportamiento
{
public:
  // =========================================================================
  // CONSTRUCTORES
  // =========================================================================

  /**
   * @brief Constructor para niveles 0, 1 y 6 (sin mapa completo)
   * @param size Tamaño del mapa (si es 0, se inicializa más tarde)
   */
  ComportamientoIngeniero(unsigned int size = 0) : Comportamiento(size)
  {
    last_action = IDLE;
    tiene_zapatillas = false;
    giros_consecutivos = 0;
    mapaVisitas = std::vector<std::vector<int>>(size, std::vector<int>(size, 0));
    ultimaFila = -1;
    ultimaCol = -1;
    turnos_sin_avanzar = 0;
    ultimaPosF = -1;
    ultimaPosC = -1;
    plan_escape = 0;
    mano_derecha = true;
    hayPlan = false;
  }

  /**
   * @brief Constructor para niveles 2, 3, 4 y 5 (con mapa completo conocido)
   * @param mapaR Mapa de terreno conocido
   * @param mapaC Mapa de cotas conocido
   */
  ComportamientoIngeniero(std::vector<std::vector<unsigned char>> mapaR,
                          std::vector<std::vector<unsigned char>> mapaC)
      : Comportamiento(mapaR, mapaC)
  {
    last_action = IDLE;
    tiene_zapatillas = false;
    giros_consecutivos = 0;
    mapaVisitas = std::vector<std::vector<int>>(mapaR.size(), std::vector<int>(mapaR[0].size(), 0));
    ultimaFila = -1;
    ultimaCol = -1;
    turnos_sin_avanzar = 0;
    ultimaPosF = -1;
    ultimaPosC = -1;
    plan_escape = 0;
    mano_derecha = true;
    hayPlan = false;
  }

  ComportamientoIngeniero(const ComportamientoIngeniero &comport)
      : Comportamiento(comport) {}

  ~ComportamientoIngeniero() {}

  /**
   * @brief Bucle principal de decisión del agente.
   * @param sensores Datos actuales de sensores.
   * @return Acción a realizar.
   */
  Action think(Sensores sensores);

  ComportamientoIngeniero *clone()
  {
    return new ComportamientoIngeniero(*this);
  }

  // =========================================================================
  // ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
  // =========================================================================

  Action ComportamientoIngenieroNivel_0(Sensores sensores);
  Action ComportamientoIngenieroNivel_1(Sensores sensores);
  Action ComportamientoIngenieroNivel_2(Sensores sensores);
  Action ComportamientoIngenieroNivel_3(Sensores sensores);
  Action ComportamientoIngenieroNivel_4(Sensores sensores);
  Action ComportamientoIngenieroNivel_5(Sensores sensores);
  Action ComportamientoIngenieroNivel_6(Sensores sensores);

protected:
  // =========================================================================
  // FUNCIONES PROPORCIONADAS
  // =========================================================================

  void ActualizarMapa(Sensores sensores);
  bool EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas);
  bool EsAccesiblePorAltura(const ubicacion &actual, bool zap);
  ubicacion Delante(const ubicacion &actual) const;
  bool es_camino(unsigned char c) const;

  void PintaPlan(const std::list<Action> &plan);
  void PintaPlan(const std::list<Paso> &plan);
  void VisualizaPlan(const ubicacion &st, const std::list<Action> &plan);
  void VisualizaRedTuberias(const std::list<Paso> &plan);

private:
  // =========================================================================
  // ESTRUCTURAS PARA BÚSQUEDA DEL NIVEL 2
  // =========================================================================

  struct EstadoI
  {
    ubicacion site;
    bool zapatillas = false;

    bool operator==(const EstadoI &other) const
    {
      return site == other.site && zapatillas == other.zapatillas;
    }

    bool operator<(const EstadoI &other) const
    {
      if (site.f != other.site.f) return site.f < other.site.f;
      if (site.c != other.site.c) return site.c < other.site.c;
      if (site.brujula != other.site.brujula) return site.brujula < other.site.brujula;
      return zapatillas < other.zapatillas;
    }
  };

  struct NodoI
  {
    EstadoI estado;
    std::list<Action> secuencia;

    bool operator==(const NodoI &other) const
    {
      return estado == other.estado;
    }

    bool operator<(const NodoI &other) const
    {
      return estado < other.estado;
    }
  };

  // =========================================================================
  // FUNCIONES AUXILIARES DE BÚSQUEDA NIVEL 2
  // =========================================================================

  EstadoI NextCasillaIngeniero(const EstadoI &st);
  EstadoI NextCasillaIngeniero2(const EstadoI &st);
  bool EnRango(int f, int c, const vector<vector<unsigned char>> &m) const;

  bool CasillaAccesibleIngeniero(const EstadoI &st,
                                 const std::vector<std::vector<unsigned char>> &terreno,
                                 const std::vector<std::vector<unsigned char>> &altura);

  bool CasillaAccesibleJumpIngeniero(const EstadoI &st,
                                     const std::vector<std::vector<unsigned char>> &terreno,
                                     const std::vector<std::vector<unsigned char>> &altura);

  EstadoI applyI(Action accion,
                 const EstadoI &st,
                 const std::vector<std::vector<unsigned char>> &terreno,
                 const std::vector<std::vector<unsigned char>> &altura);

  std::list<Action> B_Anchura_Ingeniero(const EstadoI &inicio,
                                        const ubicacion &destino,
                                        const std::vector<std::vector<unsigned char>> &terreno,
                                        const std::vector<std::vector<unsigned char>> &altura);

  // =========================================================================
  // VARIABLES DE ESTADO
  // =========================================================================

  Action last_action;
  bool tiene_zapatillas;
  int giros_consecutivos;
  std::vector<std::vector<int>> mapaVisitas;
  int ultimaFila;
  int ultimaCol;
  int turnos_sin_avanzar;
  int ultimaPosF;
  int ultimaPosC;
  int plan_escape;
  bool mano_derecha;

  std::list<Action> plan;
  bool hayPlan;
};

#endif
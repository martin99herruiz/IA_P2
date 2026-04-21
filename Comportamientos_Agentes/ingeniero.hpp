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
  // TIPOS AUXILIARES NIVEL 4
  // =========================================================================

  struct Paso
  {
    int fil;
    int col;
    int op; // -1 DIG, 0 nada, 1 RAISE
  };

  struct Nodo
  {
    int f, c;
    int h;
    int installs;
    int impacto;
    int pf, pc; // casilla anterior
    std::list<Paso> plan;
  };

  // =========================================================================
  // TIPOS AUXILIARES NIVEL 5
  // =========================================================================

  enum FaseNivel5
  {
    N5_PLANIFICAR,
    N5_PREPARAR_TRAMO,
    N5_IR_A_POSICION_ING,
    N5_LLAMAR_TECNICO,
    N5_ESPERAR_TECNICO,
    N5_AJUSTAR_TERRENO,
    N5_ALINEAR,
    N5_INSTALAR,
    N5_SIGUIENTE_TRAMO,
    N5_TERMINADO
  };

  // =========================================================================
  // CONSTRUCTORES
  // =========================================================================

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
    planNivel4Calculado = false;

    fase_n5 = N5_PLANIFICAR;
    espera_tecnico_n5 = 0;
    plan_n5_inicializado = false;
    hay_plan_mov_n5 = false;
    terreno_ajustado_n5 = false;
    tecnico_convocado_n5 = false;
    tecnico_en_posicion_n5 = false;
    tramo_instalado_n5 = false;
  }

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
    planNivel4Calculado = false;

    fase_n5 = N5_PLANIFICAR;
    espera_tecnico_n5 = 0;
    plan_n5_inicializado = false;
    hay_plan_mov_n5 = false;
    terreno_ajustado_n5 = false;
    tecnico_convocado_n5 = false;
    tecnico_en_posicion_n5 = false;
    tramo_instalado_n5 = false;
  }

  ComportamientoIngeniero(const ComportamientoIngeniero &comport)
      : Comportamiento(comport) {}

  ~ComportamientoIngeniero() {}

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
      if (site.f != other.site.f)
        return site.f < other.site.f;
      if (site.c != other.site.c)
        return site.c < other.site.c;
      if (site.brujula != other.site.brujula)
        return site.brujula < other.site.brujula;
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
  bool EnRango(int f, int c, const std::vector<std::vector<unsigned char>> &m) const;

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
  // FUNCIONES AUXILIARES DE BÚSQUEDA NIVEL 4
  // =========================================================================

  bool DentroMapa(int f, int c) const;
  bool CasillaValidaParaTuberia(int f, int c) const;
  bool OperacionValidaEnCasilla(int f, int c, int op) const;
  int ImpactoInstallCasilla(int f, int c) const;
  int ImpactoOperacionCasilla(int f, int c, int op) const;
  bool YaEnPlan(const std::list<Paso> &plan, int f, int c) const;

  std::list<Paso> BuscarPlanNivel4(const ubicacion &origen, int maxImpacto);

  // =========================================================================
  // AUXILIARES NIVEL 5
  // =========================================================================

  bool MismaCasilla(const ubicacion &u, int f, int c) const;
  bool EsAdyacenteOrtogonal(int f1, int c1, int f2, int c2) const;
  Orientacion OrientacionHacia(int f1, int c1, int f2, int c2) const;
  bool AgenteEnObjetivo(const ubicacion &u, const ubicacion &obj) const;

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

  // Niveles deliberativos previos
  std::list<Action> plan;
  bool hayPlan;
  bool planNivel4Calculado;

  // Nivel 5
  FaseNivel5 fase_n5;

  std::list<Paso> plan_tuberias_n5;
  std::list<Paso>::iterator it_tramo_n5;
  bool plan_n5_inicializado;

  Paso paso_origen_n5;
  Paso paso_destino_n5;

  ubicacion objetivo_ing_n5;
  ubicacion objetivo_tec_n5;

  std::list<Action> plan_mov_n5;
  bool hay_plan_mov_n5;

  bool terreno_ajustado_n5;
  bool tecnico_convocado_n5;
  bool tecnico_en_posicion_n5;
  bool tramo_instalado_n5;

  int espera_tecnico_n5;
};

#endif
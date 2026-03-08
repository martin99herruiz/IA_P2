#ifndef COMPORTAMIENTOINGENIERO_H
#define COMPORTAMIENTOINGENIERO_H

#include <chrono>
#include <list>
#include <map>
#include <set>
#include <thread>
#include <time.h>

#include "comportamientos/comportamiento.hpp"

// =========================================================================
// DOCUMENTACIÓN PARA ESTUDIANTES
// =========================================================================
/*
 * CLASE: ComportamientoIngeniero
 * 
 * DESCRIPCIÓN:
 * Esta clase implementa el comportamiento del agente Ingeniero en el mundo Belkan.
 * El ingeniero es responsable de explorar el mapa, encontrar objetivos y colaborar
 * con el técnico para resolver problemas.
 * 
 * NIVELES DE COMPORTAMIENTO:
 * - Nivel 0: Comportamiento reactivo básico (parcialmente implementado)
 * - Nivel 1: Comportamiento reactivo mejorado
 * - Nivel 2: Búsqueda de caminos con información completa
 * - Nivel 3: Búsqueda con información parcial
 * - Nivel 4: Planificación con múltiples objetivos
 * - Nivel 5: Colaboración básica con el técnico
 * - Nivel 6: Colaboración avanzada
 * 
 * SENSORES IMPORTANTES:
 * - sensores.superficie[0]: Tipo de terreno en la casilla actual
 * - sensores.superficie[2]: Tipo de terreno en la casilla de delante
 * - sensores.posF, sensores.posC: Posición actual (fila, columna)
 * - sensores.rumbo: Orientación actual (0=norte, 1=noreste, ..., 7=noroeste)
 * - sensores.cota[0]: Altura de la casilla actual
 * 
 * TIPOS DE TERRENO:
 * 'C' = Camino (transitable)
 * 'S' = Sendero (transitable)
 * 'D' = Zapatillas (objeto especial)
 * 'U' = Objetivo/Meta
 * 'B' = Bosque (transitable con zapatillas)
 * 'A' = Agua (no transitable)
 * 'P' = Precipicio (no transitable)
 * 'M' = Montaña (no transitable)
 * 'H' = Hierba (transitable con coste alto)
 */

struct estadoI {
  int fila;
  int columna;
  int orientacion;
  bool zap;
  bool operator<(const estadoI &n) const {
    /* 
     * TAREA DEL ESTUDIANTE: Redefinir el operador menor para usar estados en contenedores ordenados.
     * Ejemplo: std::set<estadoI> o std::map<estadoI, valor>
     * 
     */
    if (fila < n.fila) 
      return true;
    else
      return false;
  }
  bool operator==(const estadoI &n) const {
    return (fila == n.fila and columna == n.columna and
            orientacion == n.orientacion and zap == n.zap);
  }
};

struct ComparaEstadosI {
  bool operator()(const estadoI &a, const estadoI &n) const {
    /* 
     * TAREA DEL ESTUDIANTE: Functor alternativo para comparar estados.
     * Útil para colas de prioridad (priority_queue).
     * 
     */
    if ((a.fila > n.fila))
      return true;
    else
      return false;
  }
};



class ComportamientoIngeniero : public Comportamiento {
public:
  // =========================================================================
  // CONSTRUCTORES
  // =========================================================================
  
  /**
   * @brief Constructor para niveles 0, 1 y 6 (sin mapa completo)
   * @param size Tamaño del mapa (si es 0, se inicializa más tarde)
   */
  ComportamientoIngeniero(unsigned int size = 0) : Comportamiento(size) {
    // Inicializar Variables de Estado
  }

  /**
   * @brief Constructor para niveles 2, 3, 4 y 5 (con mapa completo conocido)
   * @param mapaR Mapa de terreno conocido
   * @param mapaC Mapa de cotas conocido
   */
  ComportamientoIngeniero(std::vector<std::vector<unsigned char>> mapaR, 
                         std::vector<std::vector<unsigned char>> mapaC): 
                         Comportamiento(mapaR, mapaC) {
    // Inicializar Variables de Estado
  }

  ComportamientoIngeniero(const ComportamientoIngeniero &comport)
      : Comportamiento(comport) {}
  ~ComportamientoIngeniero() {}

  /**
   * @brief Bucle principal de decisión del agente.
   * Estudia los sensores y decide la siguiente acción.
   * 
   * EJEMPLO DE USO:
   * Action accion = think(sensores);
   * return accion; // El motor ejecutará esta acción
   */
  Action think(Sensores sensores);

  ComportamientoIngeniero *clone() {
    return new ComportamientoIngeniero(*this);
  }

  // =========================================================================
  // ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
  // =========================================================================

  // Funciones específicas para cada nivel (para ser implementadas por el alumno)
  
  /**
   * @brief Implementación del Nivel 0.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_0(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 1.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_1(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 2.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */ 
  Action ComportamientoIngenieroNivel_2(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 3.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_3(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 4.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_4(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 5.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_5(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 6.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_6(Sensores sensores);

protected:
  // =========================================================================
  // FUNCIONES PROPORCIONADAS
  // =========================================================================

  /**
   * @brief Actualiza la información del mapa interno basándose en los sensores.
   * IMPORTANTE: Esta función ya está implementada. Actualiza mapaResultado y mapaCotas
   * con la información de los 16 sensores (casilla actual + 15 casillas alrededor).
   */
  void ActualizarMapa(Sensores sensores);

  /**
   * @brief Comprueba si una casilla es transitable.
   * @param f Fila de la casilla.
   * @param c Columna de la casilla.
   * @param tieneZapatillas Indica si el agente posee zapatillas.
   * @return true si la casilla es transitable (no es muro ni precipicio).
   */
  bool EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas);

  /**
   * @brief Comprueba si la casilla de delante es accesible por diferencia de altura.
   * REGLAS: Desnivel máximo 1 sin zapatillas, 2 con zapatillas.
   * @param actual Estado actual del agente (fila, columna, orientacion).
   * @return true si el desnivel con la casilla de delante es admisible.
   */
  bool EsAccesiblePorAltura(const estadoI &actual);

  /**
   * @brief Devuelve la posición (fila, columna) de la casilla que hay delante del agente.
   * @param actual Estado actual del agente (fila, columna, orientacion).
   * @return Estado con la fila y columna de la casilla de enfrente.
   */
  estadoI Delante(const estadoI &actual) const;

  bool es_camino(unsigned char c) const;

  /**
 * @brief Imprime por consola la secuencia de acciones de un plan para un agente.
 * @param plan  Lista de acciones del plan.
 */
  void PintaPlan(const list<Action> &plan);


/**
 * @brief Imprime las coordenadas y operaciones de un plan de tubería.
 * @param plan  Lista de pasos (fila, columna, operación).
 */
  void PintaPlan(const list<Paso> &plan);


  /**
 * @brief Convierte un plan de acciones en una lista de casillas para
 *        su visualización en el mapa gráfico.
 * @param st    Estado de partida.
 * @param plan  Lista de acciones del plan.
 */
  void VisualizaPlan(const estadoI &st, const list<Action> &plan);

  /**
 * @brief Convierte un plan de tubería en la lista de casillas usada
 *        por el sistema de visualización.
 * @param st    Estado de partida (no utilizado directamente).
 * @param plan  Lista de pasos del plan de tubería.
 */
  void VisualizaRedTuberias(const estadoI &st, const list<Paso> &plan);



private:
  // =========================================================================
  // VARIABLES DE ESTADO (PUEDEN SER EXTENDIDAS POR EL ALUMNO)
  // =========================================================================

};

#endif

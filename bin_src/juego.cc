#include <fstream>
#include "motorlib.hpp"

bool fileExists(std::string const &name)
{
  ifstream f(name.c_str());
  return f.good();
}

int main(int argc, char **argv)
{
  EnLinea argumentos;
  // Valores por defecto
  argumentos.ubicacion_mapa = "./mapas/mapa30.map";
  argumentos.semilla = 0;
  argumentos.level = 0;
  argumentos.posInicialIngeniero.f = -1;
  argumentos.posInicialIngeniero.c = -1;
  argumentos.posInicialIngeniero.brujula = norte;
  argumentos.posInicialTecnico.f = -1;
  argumentos.posInicialTecnico.c = -1;
  argumentos.posInicialTecnico.brujula = norte;
  argumentos.max_instantes = 3000;
  argumentos.initial_energy = 3000;
  argumentos.eco_impact_threshold = 1000;

  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " -m <mapa> -seed <semilla> -n <nivel> -i <fI> <cI> <oI> -t <fT> <cT> <oT> -Tiempo <instantes> -Energia <energia> -Ambiental <impacto> -O [fO cO]*\n";
    srand(1);
    lanzar_motor_grafico(argc, argv);
  }
  else
  {
    for (int i = 1; i < argc; i++)
    {
      if (string(argv[i]) == "-m" or string(argv[i]) == "-mapa")
      {
        argumentos.ubicacion_mapa = argv[i + 1];
        if (!fileExists(argumentos.ubicacion_mapa))
        {
          std::cout << "El mapa no existe\n";
          exit(1);
        }
        i++;
      }
      else if (string(argv[i]) == "-seed")
      {
        argumentos.semilla = atoi(argv[i + 1]);
        i++;
      }
      else if (string(argv[i]) == "-n" or string(argv[i]) == "-nivel")
      {
        argumentos.level = atoi(argv[i + 1]);
        i++;
      }
      else if (string(argv[i]) == "-i")
      {
        argumentos.posInicialIngeniero.f = atoi(argv[i + 1]);
        argumentos.posInicialIngeniero.c = atoi(argv[i + 2]);
        argumentos.posInicialIngeniero.brujula = static_cast<Orientacion>(atoi(argv[i + 3]));
        i += 3;
      }
      else if (string(argv[i]) == "-t")
      {
        argumentos.posInicialTecnico.f = atoi(argv[i + 1]);
        argumentos.posInicialTecnico.c = atoi(argv[i + 2]);
        argumentos.posInicialTecnico.brujula = static_cast<Orientacion>(atoi(argv[i + 3]));
        i += 3;
      }
      else if (string(argv[i]) == "-Tiempo")
      {
        argumentos.max_instantes = atoi(argv[i + 1]);
        i++;
      }
      else if (string(argv[i]) == "-Energia")
      {
        argumentos.initial_energy = atoi(argv[i + 1]);
        i++;
      }
      else if (string(argv[i]) == "-Ambiental")
      {
        argumentos.eco_impact_threshold = atoi(argv[i + 1]);
        i++;
      }
      else if (string(argv[i]) == "-O")
      {
         i++;
         while (i < argc and string(argv[i]).find("-") != 0) {
            pair<int, int> aux;
            aux.first = atoi(argv[i]);
            aux.second = atoi(argv[i+1]);
            argumentos.listaObjetivos.push_back(aux);
            i += 2;
         }
         i--;
      }
    }
    
    srand(argumentos.semilla);
    lanzar_motor_grafico_verOnline(argc, argv, argumentos);
  }

  exit(EXIT_SUCCESS);
}

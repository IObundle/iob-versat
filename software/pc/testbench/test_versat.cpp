#include "versat.hpp"
#include <stdio.h>
#include <fstream>

#if INFO == 1
#if nMEM > 0
void print_versat_mems(CVersat versat)
{
    FILE *f = fopen("versat_mem.hex", "w");
    for (int j = 0; j < nSTAGE; j++)
    {
        for (int l = 0; l < nMEM; l++)
        {
            fprintf(f, "\nnStage=%d \nnMEM=%d\n\n", j, l);
            for (int i = 0; i < MEM_SIZE; i++)
            {
                fprintf(f, "%d\n", versat.stage[j].memA[l].read(i));
            }
        }
    }
    fclose(f);
}
#else
void print_versat_mems(CVersat versat) {}
#endif
void print_versat_info(CVersat versat)
{
    ofstream inf;
    inf.open("versat_info.txt");
    for (int i = 0; i < nSTAGE; i++)
    {
        inf << versat.shadow_reg[i].info();
    }
    inf.close();
}

void print_versat_config(CVersat versat)
{
    for (int i = 0; i < nSTAGE; i++)
    {
        cout << versat.stage[i].info();
    }
}

ofstream inff;
int auxx = 0;
void print_versat_iter(CVersat versat)
{
    string ola;
    static int i = 0;

    if (i == 0 && versat.versat_iter == 0)
    {
        ola = "./iter/versat_iter" + to_string(versat.versat_iter) + ".txt";
        inff.open(ola);
    }
    if (i != 0 && versat.versat_iter != 0)
    {
        inff.close();
        ola = "./iter/versat_iter" + to_string(versat.versat_iter) + ".txt";
        inff.open(ola);
    }
    i++;
    string ver = "\nIteration " + to_string(versat.versat_iter) + "\n";
    for (int i = 0; i < nSTAGE; i++)
    {
        ver += versat.shadow_reg[i].info_iter();
        ver += "DATABUS  \n";
        for (int j = 0; j < 2 * (1 << (N_W - 1)); j++)
        {
            ver += "databus[" + to_string(j) + "]=" + to_string(versat.stage[i].databus[j]) + "\n";
        }
    }
    inff << ver;
    //inf.close();
}
#endif
#include "fs.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <string.h>
using namespace std;

/**
 * @brief Inicializa um sistema de arquivos que simula EXT3
 * @param fsFileName nome do arquivo que contém sistema de arquivos que simula EXT3 (caminho do arquivo no sistema de arquivos local)
 * @param blockSize tamanho em bytes do bloco
 * @param numBlocks quantidade de blocos
 * @param numInodes quantidade de inodes
 */
void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
    FILE *file = fopen(fsFileName.c_str(), "wb");
    int fileSize = 3 + ceil(numBlocks / 8.0) + sizeof(INODE) * numInodes + blockSize * numBlocks + 1;
    char zero{0x00};

    for (int i{0}; i < fileSize; i++)
    {
        fwrite(&zero, sizeof(char), 1, file);
    }

    rewind(file);

    zero = (char)blockSize;
    fwrite(&zero, sizeof(char), 1, file);

    zero = (char)numBlocks;
    fwrite(&zero, sizeof(char), 1, file);

    zero = (char)numInodes;
    fwrite(&zero, sizeof(char), 1, file);

    zero = 0x01;
    fwrite(&zero, sizeof(char), 1, file);

    char tamanhoRestante = ceil(numBlocks / 8.0);
    zero = 0x00;

    for (int i = 1; i < tamanhoRestante; i++)
    {
        fwrite(&zero, sizeof(char), 1, file);
    }

    INODE root{};
    root.IS_DIR = 0x01;
    root.IS_USED = 0x01;
    root.NAME[0] = '/';

    fwrite(&root, sizeof(INODE), 1, file);

    fclose(file);
}

/**
 * @brief Adiciona um novo arquivo dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param filePath caminho completo novo arquivo dentro sistema de arquivos que simula EXT3.
 * @param fileContent conteúdo do novo arquivo
 */
void addFile(std::string fsFileName, std::string filePath, std::string fileContent)
{
    FILE *file = fopen(fsFileName.c_str(), "rb+");

    // encontrar o índice de um inode livre
    /*char vectorinit[3];*/
    unsigned char blockSize, numBlocks, numInodes;
    fseek(file, 0, SEEK_SET);
    fread(&blockSize, sizeof(unsigned char), 1, file);
    fread(&numBlocks, sizeof(unsigned char), 1, file);
    fread(&numInodes, sizeof(unsigned char), 1, file);

    // pegando o indice do primeiro inode vazio
    unsigned char bitMapSize = (int)ceil(numBlocks / 8.0);
    fseek(file, bitMapSize + 3, SEEK_SET);

    int FirstFreeInode = 0;
    INODE inodeAux[numInodes];
    fseek(file, (3 + bitMapSize), SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inodeAux[i], sizeof(INODE), 1, file);
    }

    for (int i = 0; i < numInodes; i++)
    {
        if (inodeAux[i].IS_USED == 0)
        {
            FirstFreeInode = i;
            break;
        }
    };

    // indice dos blocos livres
    unsigned char bitMap[bitMapSize];
    fseek(file, 3, SEEK_SET);
    fread(&bitMap, sizeof(unsigned char), bitMapSize, file);

    unsigned char fileBlocks = ceil((double)fileContent.size() / (double)blockSize); // qntd de blocos que serao utilizados pelo arquivo
    int freeBlocks[fileBlocks];                                                      // vetor com blocos que serão utilizados
    unsigned char blockVectorSize = blockSize * numBlocks;
    unsigned char blockVector[blockVectorSize];
    unsigned char sizeFile = fileContent.size();
    fseek(file, -blockVectorSize, SEEK_END);
    fread(&blockVector, sizeof(unsigned char), blockVectorSize, file); // vetor de blocos
    int j = 0;

    for (int i = 1; i < blockVectorSize && j <= fileBlocks; i++) // pegar indices dos blocos livres que serão usados
    {

        if (blockVector[i] == 0)
        {
            freeBlocks[j] = i;
            j++;
        }
    }

    int usedBlocks = 0; // att valor do bitMap
    for (int i = 0; i < numBlocks; i++)
    {
        if (inodeAux[i].IS_USED == 1)
        {
            usedBlocks = +1;
        }
    }

    usedBlocks = usedBlocks + fileBlocks;

    for (int i = 0; i < usedBlocks; i++)
    {
        int blockindex = i / 8;
        bitMap[blockindex] |= (1 << i);
    }

    // ir ao inode livre
    unsigned char is_dir = 0, is_used = 1, size = fileContent.size();
    char nameDir[10], fileName[10];
    int lastIndex = filePath.find_last_of('/');
    if (lastIndex == 0)
    {
        strcpy(nameDir, "/");
        strcpy(fileName, filePath.substr(1, filePath.length()).c_str());
    }
    else
    {
        strcpy(nameDir, filePath.substr(1, lastIndex - 1).c_str());
        strcpy(fileName, filePath.substr(lastIndex + 1, filePath.length()).c_str());
        for (int i = (int(filePath.length()) - lastIndex - 1); i < 10; i++)
        {
            fileName[i] = 0;
        }
    }

    fseek(file, 3 + bitMapSize + sizeof(INODE) * FirstFreeInode, SEEK_SET);

    fwrite(&is_used, sizeof(unsigned char), 1, file);
    fwrite(&is_dir, sizeof(unsigned char), 1, file);
    fwrite(&fileName, sizeof(char), 10, file);
    fwrite(&sizeFile, sizeof(unsigned char), 1, file);
    for (int i = 0; i < fileBlocks; i++)
    {
        fwrite(&freeBlocks[i], sizeof(char), 1, file);
    }

    // Incremente SIZE do pai
    // achar indice do pai
    if (nameDir[0] != '/')
    {
        fseek(file, (3 + bitMapSize), SEEK_SET);
        for (int i = 0; i < numInodes; i++)
        {
            fread(&inodeAux[i], sizeof(INODE), 1, file);
            if (!strcmp(inodeAux[i].NAME, nameDir))
            {
                fseek(file, (3 + bitMapSize + i * sizeof(INODE) + 12), SEEK_SET);
                unsigned char fatherSize;
                fread(&fatherSize, sizeof(unsigned char), 1, file);
                fseek(file, -1, SEEK_CUR);
                fatherSize = +1;
                fwrite(&fatherSize, sizeof(unsigned char), 1, file);
                fseek(file, 3 + bitMapSize + (numInodes - 1) * sizeof(INODE) + 1 + blockSize * inodeAux[i].DIRECT_BLOCKS[0], SEEK_SET);
                fwrite(&FirstFreeInode, sizeof(char), 1, file);
                fseek(file, (3 + bitMapSize + i * sizeof(INODE) + sizeof(INODE)), SEEK_SET);
            }
        }
    }
    else
    {
        fseek(file, 3 + bitMapSize + 12, SEEK_SET);
        unsigned char fatherSize;
        fread(&fatherSize, sizeof(unsigned char), 1, file);
        fatherSize = +1;
        fseek(file, -1, SEEK_CUR);
        fwrite(&fatherSize, sizeof(unsigned char), 1, file);

        unsigned char directoryBlock[] = {1, 0};
        fseek(file, -(16), SEEK_END);
        fwrite(&directoryBlock, sizeof(unsigned char), 2, file);
    }

    // Atribua o conteúdo do arquivo aos blocos encontrados
    char content[sizeof(fileContent)];
    strcpy(content, fileContent.c_str());
    fseek(file, 1 - (16 - freeBlocks[0]), SEEK_END);
    fwrite(&content, sizeof(char), strlen(fileContent.c_str()), file);

    // att o bitMap com os blocos utilizados
    fseek(file, 3, SEEK_SET);
    fwrite(&bitMap, sizeof(unsigned char), bitMapSize, file);

    fclose(file);
}

/**
 * @brief Adiciona um novo diretório dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param dirPath caminho completo novo diretório dentro sistema de arquivos que simula EXT3.
 */
void addDir(std::string fsFileName, std::string dirPath)
{
    FILE *file = fopen(fsFileName.c_str(), "rb+");
    // encontrar o índice de um inode livre
    /*char vectorinit[3];*/
    unsigned char blockSize, numBlocks, numInodes;
    fseek(file, 0, SEEK_SET);
    fread(&blockSize, sizeof(unsigned char), 1, file);
    fread(&numBlocks, sizeof(unsigned char), 1, file);
    fread(&numInodes, sizeof(unsigned char), 1, file);

    // pegando o indice do primeiro inode vazio
    unsigned char bitMapSize = (int)ceil(numBlocks / 8.0);
    fseek(file, bitMapSize + 3, SEEK_SET);
    unsigned char bitMap[bitMapSize];
    fseek(file, 3, SEEK_SET);
    fread(&bitMap, sizeof(unsigned char), bitMapSize, file);

    int FirstFreeInode = 0;
    INODE inodeAux[numInodes];
    fseek(file, (3 + bitMapSize), SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inodeAux[i], sizeof(INODE), 1, file);
    }

    for (int i = 0; i < numInodes; i++)
    {
        if (inodeAux[i].IS_USED == 0)
        {
            FirstFreeInode = i;
            break;
        }
    };

    unsigned char fileBlocks = 1; // qntd de blocos que serao utilizados pelo diretorio
    int freeBlocks[fileBlocks];   // vetor com blocos que serão utilizados
    unsigned char blockVectorSize = blockSize * numBlocks;
    unsigned char blockVector[blockVectorSize];
    fseek(file, -blockVectorSize, SEEK_END);
    fread(&blockVector, sizeof(unsigned char), sizeof(blockVector), file); // vetor de blocos
    int j = 0;

    int usedBlocks = 0; // att valor do bitMap
    for (int i = 15; i >= 0; i--)
    {
        if (blockVector[i] != 0)
        {
            usedBlocks += 1;
        }
    }

    for (int i = 0; i < usedBlocks; i++)
    {
        int blockindex = i / 8;
        bitMap[blockindex] |= (1 << i);
    }
    fseek(file, 3, SEEK_SET);
    fwrite(&bitMap, sizeof(unsigned char), bitMapSize, file);

    // ir ao inode livre
    unsigned char is_dir = 1, is_used = 1, size = 0;
    char nameFather[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    char dirName[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int lastIndex = dirPath.find_last_of('/');
    if (lastIndex == 0)
    {
        strcpy(nameFather, "/");
        strcpy(dirName, dirPath.substr(1, dirPath.length()).c_str());
    }
    else
    {
        strcpy(nameFather, dirPath.substr(1, lastIndex - 1).c_str());
        strcpy(dirName, dirPath.substr(lastIndex + 1, dirPath.length()).c_str());
        for (int i = (int(dirPath.length()) - lastIndex - 1); i < 10; i++)
        {
            dirName[i] = 0;
        }
    }

    for (int i = 0; i < blockVectorSize; i += blockSize) // achar bloco que será utilizado
    {
        if (blockVector[i] == 0)
        {
            freeBlocks[0] = i / 2;
            break;
        }
    };

    fseek(file, 3 + bitMapSize + sizeof(INODE) * FirstFreeInode, SEEK_SET);

    fwrite(&is_used, sizeof(unsigned char), 1, file);
    fwrite(&is_dir, sizeof(unsigned char), 1, file);
    fwrite(&dirName, sizeof(char), 10, file);
    fwrite(&size, sizeof(unsigned char), 1, file);
    for (int i = 0; i < fileBlocks; i++)
    {
        fwrite(&freeBlocks[i], sizeof(char), 1, file);
    }

    // Incremente SIZE do pai
    // achar indice do pai
    if (nameFather[0] != '/')
    {
        fseek(file, (3 + bitMapSize), SEEK_SET);
        for (int i = 0; i < numInodes; i++)
        {
            fread(&inodeAux[i], sizeof(INODE), 1, file);
            if (!strcmp(inodeAux[i].NAME, nameFather))
            {
                fseek(file, (3 + bitMapSize + i * sizeof(INODE) + 12), SEEK_SET);
                unsigned char fatherSize;
                fread(&fatherSize, sizeof(unsigned char), 1, file);
                fseek(file, -1, SEEK_CUR);
                fatherSize = +1;
                fwrite(&fatherSize, sizeof(unsigned char), 1, file);
                fseek(file, 3 + bitMapSize + (numInodes - 1) * sizeof(INODE) + 1 + blockSize * inodeAux[i].DIRECT_BLOCKS[0], SEEK_SET);
                fwrite(&FirstFreeInode, sizeof(char), 1, file);
                fseek(file, (3 + bitMapSize + i * sizeof(INODE) + sizeof(INODE)), SEEK_SET);
            }
        }
    }
    else
    {
        fseek(file, 3 + bitMapSize + 12, SEEK_SET);
        unsigned char fatherSize;
        fread(&fatherSize, sizeof(unsigned char), 1, file);
        fatherSize += 1;
        fseek(file, -1, SEEK_CUR);
        fwrite(&fatherSize, sizeof(unsigned char), 1, file);

        unsigned char directoryBlock[] = {1, 2};
        fseek(file, -(16), SEEK_END);
        fwrite(&directoryBlock, sizeof(unsigned char), 2, file);
    }

    // Atribua o conteúdo do arquivo aos blocos encontrados
    char content[] = {0};
    fseek(file, 1 - (16 - 2*freeBlocks[0]), SEEK_END);
    fwrite(&content, sizeof(char), sizeof(content), file);

    fclose(file);
}

/**
 * @brief Remove um arquivo ou diretório (recursivamente) de um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param path caminho completo do arquivo ou diretório a ser removido.
 */
void remove(std::string fsFileName, std::string path)
{
    // Encontre o índice de um inode F livre conforme este slide
    // Encontre os índices dos blocos livres necessários para armazenar o conteúdo do arquivo/diretório conforme este slide
    // Atribua o conteúdo do arquivo/diretório aos blocos encontrados
    // Defina os membros de F (IS_DIR, IS_USED, NAME, SIZE, índices dos blocos encontrados, etc)
    // Incremente SIZE do pai de F
    // Atribua o índice de F no primeiro byte livre dos blocos do pai de F (na falta de byte livre, aloque um novo bloco ao pai)
}

/**
 * @brief Move um arquivo ou diretório em um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param oldPath caminho completo do arquivo ou diretório a ser movido.
 * @param newPath novo caminho completo do arquivo ou diretório.
 */
void move(std::string fsFileName, std::string oldPath, std::string newPath)
{
}

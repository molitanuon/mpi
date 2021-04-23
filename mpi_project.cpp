#include <iostream>
#include <mpi.h>
#include <fstream>
#include <vector>

using namespace std;

//Print function to show the distributed system
template <class T>
void print(vector<T> &data, int rows, int cols)
{
  cout << endl;
  for (size_t i = 0; i < rows; i++)
  {
    cout << "player " << i << ": ";
    for (int j = 0; j < cols; j++)
    {
      cout << " " << data[i * cols + j] << " ";
    }
    cout << endl;
  }
}

//read in the data the first digit is the number of players, the second is the how many scores each player has
template <class T>
void readFile(vector<T> &data, string filename, int &rows, int &cols)
{
  ifstream myfile;
  filename = "data/" + filename;
  myfile.open(filename);
  myfile >> rows >> cols;
  data = vector<int>(rows * cols, 0);

  for (size_t i = 0; i < rows; i++)
  {
    // data[i] = vector<int>(cols, 0);
    for (size_t j = 0; j < cols; j++)
    {
      myfile >> data[i * cols + j];
    }
  }
  myfile.close();
  cout << "\nScores For All Players:";
  print(data, rows, cols);
}

int main(int argc, char *argv[])
{
  //Check for valid args
  if (!argv[1])
  {
    cout << "\nERROR:\n\tNeeds filename to run: mpirun -n <Number of Players> ./mpi + <Filename>\n"
         << endl;
    exit(EXIT_FAILURE);
  }

  int rank, size;
  time_t t;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  // cout << "Hi, I am process " << rank << " of " << size << endl;

  //Vector to hold all the scores
  vector<int> allScores;

  int totalScores;
  int players = 0;
  int playerScores = 0; 

  if (rank == 0)
  {
    readFile(allScores, argv[1], players, playerScores);
    cout << "\nWe have " << players << " golf players total\n";
    cout << "We have " << playerScores << " scores per player\n";

    //P0 sends num of players, num of scores ,and totalScores to each Process
    for (size_t i = 1; i < size; i++)
    { 
      MPI_Send(&players, 1, MPI_INT, i, i, MPI_COMM_WORLD);
      MPI_Send(&playerScores, 1, MPI_INT, i, i, MPI_COMM_WORLD);
      MPI_Send(&allScores[i * playerScores], playerScores, MPI_INT, i, i, MPI_COMM_WORLD);
    }
  }
  else
  {
    MPI_Recv(&players, 1, MPI_INT, 0, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&playerScores, 1, MPI_INT, 0, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //need to allocate space for the recv buffer
    allScores.resize(playerScores, 0);
    //Processes recv their corresponding scores
    MPI_Recv(&allScores.front(), playerScores, MPI_INT, 0, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }
  //Once all processors have their corressponding local scores each can calculate a winner
  int sum = 0;
  int sum2 = 0;
  int winner = 0;

  for (size_t i = 0; i < playerScores; i++)
  {
    sum += allScores[i];
  }
  if (rank == 0)
  {
    if(size > 1)
    {
      MPI_Send(&sum, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
      MPI_Send(&winner, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
      MPI_Recv(&sum, 1, MPI_INT, size - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(&winner, 1, MPI_INT, size - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    cout << "\nPlayer " << winner << " is the winner with a total score of " << sum << endl;
  }
  else
  {
    MPI_Recv(&sum2, 1, MPI_INT, rank - 1, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&winner, 1, MPI_INT, rank - 1, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (sum2 < sum)
    {
      sum = sum2;
    }
    else
    {
      winner = rank;
    }
    MPI_Send(&sum, 1, MPI_INT, (rank + 1) % size, (rank + 1) % size, MPI_COMM_WORLD);
    MPI_Send(&winner, 1, MPI_INT, (rank + 1) % size, (rank + 1) % size, MPI_COMM_WORLD);
  }
  MPI_Finalize();
  return 0;
}

#include <fstream>
#include <iomanip>
#include <iostream>
using namespace std;

int main(int argc, char* const argv[]) {
  ifstream file;
  if (argc == 2) {
    file.open(argv[1]);
    cin.rdbuf(file.rdbuf());
  } else if (argc > 2) {
    cerr << "usage: " << argv[0] << " [filename]" << endl;
  }

  string line;
  int lineno = 0;
  while (getline(cin, line)) {
    cout << "111111111" << endl;
    cout << setw(4) << setfill(' ') << ++lineno << ' ' << line << endl;
  }
  cout << "22222222222" << endl;
  return 0;
}
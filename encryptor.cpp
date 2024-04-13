#include <fstream>
#include <iostream>
#include <random>
#include <string>

using namespace std;

void exit_dialogue() { cout << "Exitting program..." << endl; }

void invalid_choice_dialogue() {
  cout << "Invalid choice. Asking again..." << endl;
}

void warning_dialogue(string process) {
  cout << endl
       << "Warning: Ensure the " << process
       << " process is not interupted, "
          "especially from a power failure."
       << endl
       << "Notice: This program will begin searching in the active directory. "
          "Enter `exit` to exit the program."
       << endl;
}

bool get_file_path(string &input_file_path, string ask_message) {
  bool file_retry = true;
  ifstream input_file_test;

  while (file_retry) {
    cout << ask_message;
    getline(cin, input_file_path);

    if (input_file_path == "exit") {
      exit_dialogue();
      file_retry = false;
    } else {
      input_file_test.open(input_file_path, ios::binary);

      if (input_file_test.is_open()) {
        cout << "`" << input_file_path << "` found!" << endl;
        file_retry = false;
      } else {
        cout << "Could not find file: `" << input_file_path
             << "`. Asking again..." << endl;
      }
      input_file_test.close();
    }
  }

  if (input_file_path == "exit") {
    return false;
  }

  return true;
}

void encrypt(string file_path) {
  ifstream file;
  ofstream part_1_file, part_2_file;
  char file_byte, new_byte;

  random_device entropy_source;
  default_random_engine generator(entropy_source());
  uniform_int_distribution<> random_byte(-127, 127);

  cout << endl << "Starting encryption..." << endl;

  file.open(file_path, ios::binary);
  part_1_file.open(file_path + ".pt_1", ios::binary);
  part_2_file.open(file_path + ".pt_2", ios::binary);

  /*
  For each byte in our original file, generate a random byte, then have one file
  contain these random bytes and the other file contain the original byte XOR
  the random byte. In another way, part_1 is our "encrypted" file, while part_2
  is our "key". The XOR means that the order/distinction doesn't mather.
  */
  while (file.get(file_byte)) {
    new_byte = random_byte(generator);
    part_1_file.put(new_byte ^ file_byte);
    part_2_file.put(new_byte);
  }

  file.close();
  part_1_file.close();
  part_2_file.close();

  cout << "Finished encryption. Output files: `" << file_path + ".pt_1"
       << "`, `" << file_path + ".pt_2`" << endl;
}

void decrypt(string part_1_file_path, string part_2_file_path) {
  ifstream part_1_file, part_2_file;
  ofstream decrypted_file;
  char part_1_file_byte, part_2_file_byte;
  int part_1_file_length, part_2_file_length, decryption_method = 1;
  bool error_retry = true, exiting = false;
  string choice;

  cout << endl << "Starting decryption..." << endl;

  part_1_file.open(part_1_file_path, ios::binary);
  part_2_file.open(part_2_file_path, ios::binary);

  /*
  These lines check the length of the files by sending the file stream pointer
  to the end of the file, then reading its position. This is to avoid using
  <filesystem>, since it is still experimental in LLVM/clang. Technically, the
  lines with `.end` are not needed if I had included `ios::ate` to the `.open()`
  statements above. However, I think the below code looks more explicit.
  */
  part_1_file.seekg(0, part_1_file.end); // ^ look above
  part_1_file_length = part_1_file.tellg();
  part_1_file.seekg(0, part_1_file.beg);
  part_2_file.seekg(0, part_2_file.end); // ^ look above
  part_2_file_length = part_2_file.tellg();
  part_2_file.seekg(0, part_2_file.beg);

  if (part_1_file_length != part_2_file_length) {
    cout << "Error: `" << part_1_file_path
         << "` does not have the same size as `" << part_2_file_path
         << "`. Decryption will not be completely effective." << endl
         << "Enter `1` to attempt decryption, up to the last usuable byte."
         << endl
         << "Enter `2` to attempt decryption, allowing excess encrypted bytes "
            "to be seen."
         << endl
         << "Type `3` to exit the program." << endl;

    while (error_retry) {
      cout << "Enter your choice: ";
      getline(cin, choice);

      error_retry = false;
      switch (stoi(choice)) {
      case 1:
        /* It is already `1` by default. This is to prevent the input from being
         * included by the `default` case. */
        break;
      case 2:
        // Reasoning for ternary if statement is in the switch statement below.
        decryption_method = (part_1_file_length > part_2_file_length) ? 2 : 3;
        break;
      case 3:
        exit_dialogue();
        exiting = true;
        break;
      default:
        error_retry = true;
        invalid_choice_dialogue();
      }
    }
  }

  if (exiting) {
    part_1_file.close();
    part_2_file.close();
    return;
  }

  decrypted_file.open(
      part_1_file_path + ".decrypted",
      ios::binary); /* I think it's fine to write the decrypted file as just the
                       first file with another extention, since the files could
                       be renamed after being encrypted anyways. */

  switch (decryption_method) {
  case 1:
    /* Since the files are initially assumed to be the same size, it would
     * technically be okay for me to use the code for case 2 or case 3, as there
     * would be no excess information, making the `&&` redundant. However,
     * allowing this method does skip the ternary if statement, saving time. */
    while (part_1_file.get(part_1_file_byte) &&
           part_2_file.get(part_2_file_byte)) {
      decrypted_file.put(part_1_file_byte ^ part_2_file_byte);
    }
    break;
  case 2:
    /* If part_1 has more information than part_2, then XOR the excess
     * information with `0b00000000`.*/
    while (part_1_file.get(part_1_file_byte)) {
      decrypted_file.put(part_1_file_byte ^
                         ((part_2_file.get(part_2_file_byte)) ? part_2_file_byte
                                                              : 0b00000000));
    }
    break;
  case 3:
    /* If part_2 has more information than part_1, then XOR the excess
     * information with `0b00000000`.*/
    while (part_2_file.get(part_2_file_byte)) {
      decrypted_file.put(((part_1_file.get(part_1_file_byte)) ? part_1_file_byte
                                                              : 0b00000000) ^
                         part_2_file_byte);
    }
    break;
  }

  part_1_file.close();
  part_2_file.close();
  decrypted_file.close();

  cout << "Finished decryption. Output files: "
       << part_1_file_path + ".decrypted" << endl;
}

void encrypt_dialogue() {
  string input_file_path;
  ifstream input_file;

  warning_dialogue("encryption");

  if (get_file_path(input_file_path, "Which file do you want to encrypt?: ")) {
    encrypt(input_file_path);
  };
}

void decrypt_dialogue() {
  string input_part_1_file_path, input_part_2_file_path;
  ifstream input_file;

  warning_dialogue("decryption");

  if (get_file_path(input_part_1_file_path,
                    "Enter the path of either the `.pt_1` or `.pt_2` file: ")) {
    if ((get_file_path(input_part_2_file_path,
                       "Enter the path of the other `.pt_*` file: "))) {
      decrypt(input_part_1_file_path, input_part_2_file_path);
    }
  }
}

int main() {
  string choice;
  int number_choice;
  bool choice_retry = true;

  cout << "Enter `1` for file encryption." << endl
       << "Enter `2` for file decryption." << endl
       << "Enter `3` to exit program." << endl;

  while (choice_retry) {
    cout << "Enter your choice here: ";
    getline(cin, choice);

    try {
      number_choice = stoi(choice);
    } catch (exception &cannot_convert) {
      number_choice = 0;
    }

    choice_retry = false;
    switch (number_choice) {
    case 1:
      encrypt_dialogue();
      break;
    case 2:
      decrypt_dialogue();
      break;
    case 3:
      exit_dialogue();
      break;
    default:
      choice_retry = true;
      invalid_choice_dialogue();
    }
  }

  return 0;
}
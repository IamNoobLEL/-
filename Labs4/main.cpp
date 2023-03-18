#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <string>

int main()
{
  printf("Ты в родительском процессе с id [%i]\n", getpid());

  char symbol, *in = (char *)malloc(sizeof(char)), *file_path = (char *)malloc(sizeof(char));
  int *size = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0), counter = 0; /*тыкаем флаги мэпы для наших процессов*/
  if (size == MAP_FAILED) /*успешность создания области памяти*/
  {
    printf("Ошибка в создании целочисленного значения при выполнении отображения памяти\n");
    exit(1);
  }
  *size = 1;

  printf("Введите путь к файлу\n");
  /*читаем путь к файлу*/
  while ((symbol = getchar()) != '\n')
  {
    file_path[counter++] = symbol;
    if (counter == *size)
    {
      *size *= 2;
      file_path = (char *)realloc(file_path, (*size) * sizeof(char));
    }
  }
  /*расширяем память*/
  file_path = (char *)realloc(file_path, (counter + 1) * sizeof(char));
  file_path[counter] = '\0';
  counter = 0, *size = 1;
  printf("Теперь введите какие-то строчки. Если хотите закончить ввод нажмите Ctrl+D\n");
  /*считываем символы*/
  while ((symbol = getchar()) != EOF)
  {
    in[counter++] = symbol;
    if (counter == *size)
    {
      *size *= 2;
      in = (char *)realloc(in, (*size) * sizeof(char));
    }
  }

  *size = counter + 1;
  in = (char *)realloc(in, (*size) * sizeof(char));
  in[(*size) - 1] = '\0';
  char *ptr = (char *)mmap(NULL, (*size) * sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

  if (ptr == MAP_FAILED) /*проверяем,  удалось ли отобразить виртуальную память для массива символов*/
  {
    printf("Ошибка маппинга при создании массива символов\n");
    free(in);
    free(file_path);
    int err = munmap(size, sizeof(int));
    if (err != 0)
    {
      printf("Мэппинг не удался\n");
    }
    exit(1);
  }

  strcpy(ptr, in); /*копирует содержимое строки in в область памяти, на которую указывает ptr */
  int fd = open(file_path, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO); /*устанавливают права на чтение, запись и выполнение для всех типов пользователей*/

  if (fd == -1) /*проверка на успешное открытие файла*/
  {
    printf("Ошибка в открытии файла\n");
    free(in);
    free(file_path);
    int err1 = munmap(ptr, (*size) * sizeof(char));
    int err2 = munmap(size, sizeof(int));
    if ((err1 != 0) || (err2 != 0))
    {
      printf("Анмэппинг не удался\n");
    }
    exit(1);
  }

  char *f = (char *)mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); /*выполняет отображение файла, открытого с помощью функции open(), в память процесса, используя функцию mmap()*/

  if (f == MAP_FAILED) /*проверяем успешность*/
  {
    printf("Не удалось создать строку, связанную с файлом\n");
    free(in);
    free(file_path);
    int err1 = munmap(ptr, (*size) * sizeof(char));
    int err2 = munmap(size, sizeof(int));
    if ((err1 != 0) || (err2 != 0))
    {
      printf("Анмэппинг не удался\n");
    }
    exit(1);
  }

  pid_t child_pid = fork();

  if (child_pid == -1) /*проверяем успех создания дочернего процесса*/
  {
    printf("Ошибка в создании дочернего процесса\n");
    free(in);
    free(file_path);
    int err1 = munmap(ptr, (*size) * sizeof(char));
    int err2 = munmap(size, sizeof(int));
    if ((err1 != 0) || (err2 != 0))
    {
      printf("Анмэппинг не удался\n");
    }
    exit(1);
  }

  else if (child_pid == 0)
  {
    //child
    printf("Ты в дочернем процессе с id [%i]\n", getpid());
    std::string string = std::string(), file_string = std::string(), out_string = std::string();
    for (int i = 0; i < *size; i++) /*обходим строки и суем в переменные*/
    {
      if (i != (*size) - 1)
      {
        string += ptr[i];
      }
      if ((ptr[i] == '\n') || (i == (*size) - 1))
      {
        if ((i > 0) && ((ptr[i - 1] == '.') || (ptr[i - 1] == ';')))
        {
          file_string += string;
        }
        else
        {
          out_string += string;
        }
        string = std::string();
      }
    }
    if ((file_string.length()) && (file_string[file_string.length() - 1] != '\n')) /*добавляет символ перевода строки в конец строки file_string, если последний символ в ней не является символом перевода строки*/
    {
      file_string += '\n';
    }
    /*проверяем, что строка, содержащая текст для записи в файл, не пустая. 
    Если она не пустая, то мы обрезаем файл до необходимого размера, переразмещаем память для строки, связанной с файлом, и записываем 
    строку в эту память*/
    if (file_string.length() != 0)
    {
      if ((ftruncate(fd, std::max((int)file_string.length(), 1) * sizeof(char))) == -1)
      {
        printf("Не удалось обрезать файл\n");
        free(in);
        free(file_path);
        return 1;
      }
      if ((f = (char *)mremap(f, sizeof(char), (file_string.length() + 1) * sizeof(char), MREMAP_MAYMOVE)) == ((void *)-1))
      {
        printf("Не удалось изменить размер выделенной памяти для строки, связанной с файлом\n");
        free(in);
        free(file_path);
        return 1;
      }
      sprintf(f, "%s", file_string.c_str());
    }
    if ((out_string.length()) && (out_string[out_string.length() - 1] != '\n')) /*доб-ем символ перевода строки в конец, если его нет*/
    {
      out_string += '\n';
    }
    /*уменьшает размер выделенной области памяти, на которую указывает указатель ptr, до длины строки аут строки + 1*/
    if ((ptr = (char *)mremap(ptr, (*size) * sizeof(char), out_string.length() + 1, MREMAP_MAYMOVE)) == ((void *)-1))
    {
      printf("Не удалось обрезать файл для строки\n");
      free(in);
      free(file_path);
      return 1;
    }
    *size = out_string.length() + 1;
    sprintf(ptr, "%s", out_string.c_str());
  }
  else
  {
    //parent
    /*ждем конца работы дочернего и после продолжаем в родительском*/
    int wstatus;
    waitpid(child_pid, &wstatus, 0);
    if (wstatus)
    {
      free(in);
      free(file_path);
      int err1 = munmap(ptr, (*size) * sizeof(char));
      int err2 = munmap(f, counter * sizeof(char));
      int err3 = munmap(size, sizeof(int));
      if ((err1 != 0) || (err2 != 0) || (err3 != 0))
      {
        printf("Анмэппинг не удался\n");
      }
      exit(1);
    }
    printf("Ты вернулся в родительский процесс с id [%i]\n", getpid());
    
    struct stat statbuf; /*получение информации о размере файла, вывод содержимого двух строк и закрытие файлового дескриптора*/
    if (fstat(fd, &statbuf) < 0)
    {
      printf("Проблемы с открытием файла %s\n", file_path);
      exit(1);
    }
    counter = std::max((int)statbuf.st_size, 1);
    printf("Эти строки оканчиваются на '.' или ';':\n");
    if (statbuf.st_size > 1)
    {
      printf("%s", f);
    }
    printf("------------------------------------------------------------------------------\nЭти строки не оканчиваются на '.' или ';':\n");
    printf("%s", ptr);
    close(fd);

    int err1 = munmap(ptr, (*size) * sizeof(char)); /*освобождает выделенную память и завершает работу программы*/
    int err2 = munmap(f, counter * sizeof(char));
    int err3 = munmap(size, sizeof(int));

    if ((err1 != 0) || (err2 != 0) || (err3 != 0))
    {
      printf("Анмэппинг не удался\n");
      free(in);
      free(file_path);
      exit(1);
    }
  }
  free(in);
  free(file_path);
  return 0;
}

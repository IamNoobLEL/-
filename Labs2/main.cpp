#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
int main()
{
	int fd1[2], fd2[2];
	if ((pipe(fd1) == -1) || (pipe(fd2) == -1)) /* чекаем что дескрипторы рабочие */
	{
		std::cout << "Ошибка в открытии pipe`s между родительским и дочерним процессами" << std::endl;
		exit(1);
	}

	pid_t child;
	if ((child = fork()) == -1) /* чекаем что дочерний процесс рабочий */
	{
		std::cout << "Ошибка в создании дочернего процесса" << std::endl;
		exit(1);
	}

	else if (child > 0) /* если рабочий, то ныряем сюда */
	{
		std::cout << "Ура, ты в родительском процессе с id [" << getpid() << "]" << std::endl;
		std::string file_path, string; /*создаем строчку */

		std::cout << "Введите путь к файлу" << std::endl;
		getline(std::cin, file_path); /*считываем строчку */

		int length = file_path.length() + 1;
		write(fd1[1], &length, sizeof(int));
		write(fd1[1], file_path.c_str(), length * sizeof(char)); /*сколько занимает в байтак. штук справа умножается на длину */

		char symbol, *in = (char*) malloc (2 * sizeof(char)); /*маллок создает массив и задаем сколько байт в нем будет */
		int counter = 0, size_of_in = 2;
		std::cout << "Введите какие-то строчки. Если хотитет закончить, нажмите Ctrl+D" << std::endl;

		while ((symbol = getchar()) != EOF)
		{
			in[counter++] = symbol;
			if (counter == size_of_in)
			{
				size_of_in *= 2;
				in = (char*) realloc (in, size_of_in * sizeof(char));
			}
		}

		in = (char*) realloc (in, (counter + 1) * sizeof(char)); /* риалок увеличивает либо уменьшает массив */
		in[counter] = '\0';

		write(fd1[1], &(++counter), sizeof(int));
		write(fd1[1], in, counter * sizeof(char));
		close(fd1[0]);
		close(fd1[1]);

		int out_length, wstatus;
		waitpid(child, &wstatus, 0);

		if (wstatus)
		{
			close(fd2[0]);
			close(fd2[1]);
			free(in);
			exit(1);
		}

		std::cout << "Ура, ты вернулся в родительский процесс с id [" << getpid() << "]" << std::endl;

		read(fd2[0], &out_length, sizeof(int));
		char* out = (char*) malloc (out_length); /*пон */
		read(fd2[0], out, out_length * sizeof(char));

		std::ifstream fin(file_path.c_str()); /*делает поток из файла чтобы можно было считывать */

		if (!fin.is_open())
		{
			close(fd2[0]);
			close(fd2[1]);
			free(in);
			std::cout << "Ошибка в открытии файла, чтобы прочитать строки" << std::endl;
			exit(1);
		}

		std::cout << "------------------------------------------------" << std::endl << "Эти строки оканчиваются на '.' или ';' :" << std::endl;

		if (fin.peek() != EOF)
		{
			while (!fin.eof())
			{
				getline(fin, string);
				std::cout << string << std::endl;
			}
		}
		fin.close();

		std::cout << "------------------------------------------------" << std::endl << "Эти строчки не оканчиваются на '.' или ';' :" << std::endl;

		for (int i = 0; i < out_length - 1; i++)
		{
			std::cout << out[i];
		}
		close(fd2[0]);
		close(fd2[1]);
		free(in);
		free(out);
	}

	else
	{
		int length;
		read(fd1[0], &length, sizeof(int)); /*из шняги считывает в переменну. длину */

		char* c_file_path = (char*) malloc (length * sizeof(char)); /* массив создали */
		read(fd1[0], c_file_path, length * sizeof(char)); /*до этого создали стрчоку и ту считали */

		int counter;
		read(fd1[0], &counter, sizeof(int));

		char* inc = (char*) malloc (counter * sizeof(char));
		read(fd1[0], inc, counter * sizeof(char));

		close(fd1[0]);
		close(fd1[1]);

		std::cout << "Ура, ты в дочернем процессе с id [" << getpid() << "]" << std::endl;
		std::string string = std::string(), out_string = std::string(), file_string = std::string(); /*создали 3 пустые строки */

		/*тут чекаем строчки на выполнение условий. Начинаем с проверки не на конец строчки, суем в стринг символ. 
		Дальше проверяем если конец строки и перенос строки. 
		Если да, то внутрь. 
		Если предпоследний символ наше условие, то пихаем строку в ответ. Иначе пихаем в другое. И зануляем.*/

		for (int i = 0; i < counter; i++)
		{
			if (inc[i] != '\0')
			{
				string += inc[i];
			}
			if ((inc[i] == '\n') || (inc[i] == '\0'))
			{
				if ((i > 0) && ((inc[i - 1] == '.') || (inc[i - 1] == ';')))
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

		if ((file_string.length()) && (file_string[file_string.length() - 1] == '\n'))
		{
			file_string.pop_back();
		}

		std::ofstream fout(c_file_path); /*поток чтобы выводить */

		if (!fout.is_open()) /* если файл не открывается */
		{
			std::cout << "Ошибка в создании или открытии файла, чтобы записать строчки" << std::endl;
			free(inc);
			free(c_file_path);
			close(fd2[0]);
			close(fd2[1]);
			return 1;
		}

		int out_length = out_string.length() + 1;

		write(fd2[1], &out_length, sizeof(int));
		write(fd2[1], out_string.c_str(), out_length * sizeof(char));
		close(fd2[0]);
		close(fd2[1]);
		
		if (!file_string.empty()) /* если файл не пустой */
		{
			fout << file_string; /* в вывод пихаем то что нашли */
		}
		fout.close();
		free(inc);
		free(c_file_path);
	}
	return 0;
}

ОС ИДЗ 3

АБДУЛЛИН САИД МАРАТОВИЧ БПИ 218

Вариант 36

Задание: Задача о сельской библиотеке. В библиотеке имеется N книг, Каждая из книг в одном экземпляре. M читателей регулярно заглядывают в библиотеку, выбирает для чтения одну книгу и читает ее некоторое количество дней. Если желаемой книги нет, то читатель дожидается от библиотекаря информации об ее появлении и приходит в библиотеку, чтобы специально забрать ее. Возможна ситуация, когда несколько читателей конкурируют из-за этой популярной книги. Создать приложение, моделирующее заданный процесс. Библиотекарь и читатели должны быть представлены в виде отдельных процессов.

Каждый новый клиент получает свой поток, в котором происходит его взаимодействие с сервером.
Клиенты бывают типа читателя(отправляют 0 при подключении к серверу) и типа монитора(отправляют 1).
Информация в монитор сохраняется в буффер, не забывая защитить буффер семафорами, после чего передается мониторам.
Задача выполнена по сути на 8 баллов, т.е. существует возможность подключать несколько мониторов и клиентов к серверу. Однако почему-то происходит какая-то ошибка и монитору иногда посылается мусор, что однако не мешает видеть ход обмена сервера и клиентов, из-за чего считаю работу достойной 7 баллов.
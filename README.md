***Задача № 232***

В данной задаче я реализовала программу "сервер" и программу "клиент". К одному серверу могут подключаться несколько 
клиентов. Каждый клиент отправляет некоторые текстовые данные, которые логгируются с сервером(*с указанием id клиента*). 
Для запуска программы воспользуйтесь следующими командами:
1. Для запуска сервера:
```
make run_server
```
2. Для запуска клиента:
```
make run_client
```
3. Для удаления кеша:
```
make delete_build_cache
```
Для окончания работы с сервером или клиентом введите в консоль Ctrl+C или отправьте соответвующий сигнал.

*Номер порта или сервера можно поменять в файле Makefile. По умолчанию логи сохраняются в "Clients_logs.txt"*

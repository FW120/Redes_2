Compilação client:

gcc client.c -o client -lpthread

Compilação server:

gcc server.c -o server -lpthread

Execução:
Execute primeiramente o server na maquina a qual o server ficará, após a execução digite a porta que o server utilizará. Para o client, execute-o em outra maquina depois que o server já estiver em execução, e então digite as informações solicitadas (nome de usuario, porta e ip do server)
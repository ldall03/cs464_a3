main: server.c
	@echo "Compiling server"
	@echo "  Assignment submitted by: Loic Dallaire"
	@echo "  Student ID: 002311806"
	@echo ""
	gcc server.c -lpthread -o shfd

client: client.c
	@echo "Compiling client"
	@echo ""
	gcc client.c -o client

clean:

#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <string.h>
#define MAX 100
#define QUERY_MAX 1000
#define ENTER(S) printf("Please enter %s:\n", S)


PGconn *startConnection(const char* conn_info);
int display(PGconn *connection, char *query, int parcount, const char **params);
PGresult *update(PGconn *c, char *query, int parcount, const char **params);
void lootabel_margid(PGconn *c);
void lootabel_autod(PGconn *c);
void lootabel_omanikud(PGconn *c);
void lootabel_omanikud_ajalugu(PGconn *c);
char *readText(char *name);
char *readInt(char *name);
char *readNum(char *name);
void printmenu();

int main()
{
    PGconn *connection = startConnection("dbname=prog22 host=localhost port=54321 user=BLANK password=BLANK"); // startConnection string user and pass removed
    if(connection == NULL){                                                                                    // for security reasons
        printf("No connection\n");
        return 0;
    }
    lootabel_margid(connection);              
    lootabel_autod(connection);
    lootabel_omanikud(connection);
    lootabel_omanikud_ajalugu(connection);
    int menu = 1;
    char **params = malloc(20 * sizeof(char *)); //Varuga
    char buf[MAX];
	printmenu();
    while(menu){
        char query[QUERY_MAX];
        char *select;
        fgets(buf, MAX, stdin);
        switch(buf[0]){
        case'1':
            printf("1: Display brands\n"
                   "2: Display cars\n"
                   "3: Display owners\n"
                   "4: Display history\n");
            select = readInt("your option");
            switch(select[0]){
                case'1':
                    printf("\t\t\t\t---===CAR BRANDS===---\n");
                    display(connection, "select * from margid",0, NULL);
                    break;
                case'2':
                    printf("\t\t\t\t\t\t\t\t\t\t---==CARS==---\n");
                    display(connection, "select * from autod order by id",0, NULL);
                    break;
                case'3':
                    printf("\t\t\t\t\t---===OWNERS===---\n");
                    display(connection, "select * from omanikud",0, NULL);
                    break;
                case'4':
                    printf("\t\t\t\t\t\t\t\t---===HISTORY===---\n");
                    display(connection,
                    "select ajalugu.car_id, autod.name, autod.model, omanikud.name, start, stop from ajalugu "
                    "inner join autod on ajalugu.car_id = autod.id "
                    "inner join omanikud on ajalugu.owner_id = omanikud.id "
                    "order by car_id desc",0, NULL);
                    break;
                default:
                    printf("Did not recognize command\n");
                    break;
                }
                break;
        case '2':
            params[0] = readText("name");
            params[1] = readText("country");
            if(strlen(params[0]) == 0 || strlen(params[1]) == 0){
                printf("Name or country cannot be empty!");
                break;
            }
            strcpy(query, "insert into margid(name, country) values($1, $2)");
            update(connection, query, 2, (const char **)params);
            break;
        case '3':
            params[0] = readText("brand name");
            params[1] = readText("model");
            params[2] = readInt("price");
            params[3] = readInt("year produced between 1886-2022");
            params[4] = readInt("engine power(kW) between 50-1200");
            params[5] = readNum("engine capacity (L) between 0.5 and 12");
            strcpy(query, "insert into autod(name, model,price, produced, engine_power, engine_capacity) "
                   "values($1, $2, $3, $4, $5, $6)");
            update(connection, query, 6, (const char **)params);
            break;
        case'4':
            params[0] = readText("owner name");
            params[1] = readText("city");
            params[2] = readText("country");
            strcpy(query, "insert into omanikud(name, city, country) "
                   "values($1, $2, $3)");
            update(connection, query, 3, (const char **) params);
            break;
        case'5':
            char *answer = readText("Y or y if you would you like to see all car and owner IDs listed?");
            if (strlen(answer) == 1 && (strcmp(answer, "Y")==0 || strcmp(answer, "y")==0)){
                display(connection, "select * from autod order by id",0, NULL);
                printf("\n");
                display(connection, "select * from omanikud order by id",0, NULL);
            }
            params[0] = readInt("car id where you would like to add an owner");
            params[1] = readInt("the owner's id who you would like to add");
            printf("Start date cannot be before year produced or listing will be deleted\n");
            params[2] = readText("start of ownership");
            strcpy(query,
                   "insert into ajalugu(car_id, owner_id,start) "
                   "values((select id from autod where id = $1), (select id from omanikud where id = $2), $3)");
            if(PQresultStatus(update(connection, query, 3, (const char**)params)) == PGRES_FATAL_ERROR)
                break;

            params[1]=params[2];
            strcpy(query,
                   "delete from ajalugu "
                   "where extract(year from $2::timestamp) < "
                   "(select produced from autod where id = $1) "
                   "and start = $2");
            update(connection, query, 2, (const char**)params);
            strcpy(query,
                   "update ajalugu "
                   "set stop = $2 "
                   "where car_id = $1 "
                   "and start <> $2 "
                   "and DATE_PART('day', stop-start) is NULL "
                   "and stop is null ");
            update(connection, query, 2, (const char**)params);
            break;
        case '6':
            printf("\n--== Select ==--\n"
                    "1: Search for owners name, select car and change price\n"
                    "2: Search for car model, select id and change price\n");

            select = readText("1 or 2");
            if (!(strlen(select) == 1 && (strcmp(select, "1") == 0 || strcmp(select, "2") == 0))){
                printf("Only 1 or 2!\n");
                ungetc('\n', stdin);
                ungetc('6', stdin);
                break;
            }
            if (atoi(select)==1){ //Valiti 1, otsitakse omaniku nime jargi
                params[2] = readText("owners full name");
                sprintf(query, "select distinct(a.car_id), c.name,c.model,c.price, a.owner_id, b.name from ajalugu a "
                       "inner join omanikud as b on a.owner_id = b.id "
                       "inner join autod as c on a.car_id = c.id "
                       "where (a.stop is null "
                       "and b.name = '%s' )"
                       "", params[2]);
                if(!(display(connection, query, 0, NULL))){
                    printmenu();
                    break;
                }
                params[0] = readInt("the car_id whose price you would like to change");
                params[1] = readInt("cars new price");
                sprintf(query,
                       "update autod "
                       "set price = $2 "
                       "from omanikud, ajalugu "
                       "where autod.id = $1 "
                       "and ajalugu.car_id = $1 "
                       "and ajalugu.owner_id = (select omanikud.id from omanikud where omanikud.name = '%s')", params[2]);
                update(connection, query,2, (const char **)params);
            }
            else{   //Valiti 2, otsitakse mudeli jargi

                params[2] = readText("partially or fully the name of the car model");
                sprintf(query, "select id, name, model, price from autod "
                        "where model like '%%%s%%'", params[2]);

                if(!(display(connection, query, 0, NULL))){
                    printmenu();
                    break;
                }
                params[0] = readInt("id of the car whose price you would like to change");
                params[1] = readInt("the new price");
                sprintf(query,
                        "update autod "
                        "set price = $2 "
                        "where id = $1 "
                        "and model like '%%%s%%'",params[2]);
                update(connection, query, 2, (const char **)params);
            }
            break;

        case'7':
            params[0] = readInt("year");
            params[1] = readText("model");
            sprintf(query,
                    "select * from autod "
                    "where produced = $1 "
                    "and model like '%%%s%%'"
                    "order by price desc", params[1]);
            display(connection, query, 1, (const char **)params);
            break;

        case'8':
            printf("Show cars with capacity\n"
                   "1: Bigger than [ ]\n"
                   "2: Smaller than [ ]\n"
                   "3: Between [ ] and [ ]\n");

            select = readInt("your option");

            switch(select[0]){
            case'1':
                params[0] = readNum("capacity 0.5 - 12 (L)");
                if(atof(params[0]) >= 0.5 && atof(params[0]) <= 12){
                    strcpy(query,"select * from autod "
                           "where engine_capacity > $1 "
                           "order by engine_capacity asc");
                    display(connection, query, 1, (const char**)params);
                }
                else
                    printf("Range is 0.5 - 12!\n");
                break;

            case'2':
                params[0] = readNum("capacity 0.5 - 12 (L)");
                if(atof(params[0]) >= 0.5 && atof(params[0]) <= 12){
                    strcpy(query,"select * from autod "
                           "where engine_capacity < $1 "
                           "order by engine_capacity desc");
                    display(connection, query, 1, (const char**)params);
                }
                else
                    printf("Range is 0.5 - 12!\n");
                break;

            case'3':
                params[0] = readNum("minimum capacity 0.5 - 12 (L)");
                params[1] = readNum("maximum capacity 0.5 - 12 (L)");
                if(atof(params[0]) >= 0.5 && atof(params[0]) <= 12 && atof(params[1]) >= 0.5 && atof(params[1]) <= 12 && atof(params[0]) <= atof(params[1])){
                    strcpy(query,"select * from autod "
                           "where engine_capacity between $1 and $2 "
                           "order by engine_capacity asc");
                    display(connection, query, 2, (const char**)params);
                }
                else
                    printf("\nYou have entered a faulty value\n");
                break;
            default:
                printf("Only 1, 2 or 3!\n");
                break;
            }
            break;
        case'9':
            strcpy(query,
                   "select cast(avg(b.price)as decimal(10,2)) as average,margid.name from margid "
                   "inner join autod b on margid.name = b.name "
                   "where b.status = 'on_sale'"
                   "group by margid.name order by average asc");
            display(connection, query, 0, NULL);
            break;
        case'A':
            strcpy(query,
                   "select b.city, count(distinct(car_id)) as count from ajalugu "
                   "inner join omanikud b on owner_id = b.id "
                   "where ajalugu.stop is NULL "
                   "group by b.city order by count desc");
            display(connection, query, 0, NULL);
            break;
        case'X':
            menu = 0;
            break;
        case 'H':
            printmenu();
            break;
        default: printf("Did not recognize command\n");
        }

    }
    PQfinish(connection);
    free(params);
    return 0;
}

PGconn *startConnection(const char* conn_info){
    PGconn *connection;
    connection = PQconnectdb(conn_info);
    if(PQstatus(connection) == CONNECTION_OK){
        printf("Successfully connected to database\n");
        return connection;
    }
    else
        return 0;
}

void lootabel_margid(PGconn *c){
    PGresult *res;
    res= PQexec(c, "Create table if not exists margid"
                    "(id serial PRIMARY KEY,"
                    "name varchar(20) unique not null,"
                    "country varchar(20) not null)");
    if(PQresultStatus(res) != PGRES_TUPLES_OK){
        printf("%s", PQresultErrorMessage(res));
        return;
    }
    PQclear(res);
}

void lootabel_autod(PGconn *c){
    PGresult *res;
    res = PQexec(c, "Create table if not exists autod"
                 "(id serial PRIMARY KEY,"
                 "name text REFERENCES margid(name),"
                 "model text not null,"
                 "produced int not null, check(produced between 1886 and 2022)," //Carl Benz created 1st car 1886
                 "engine_power int not null, check(engine_power between 50 and 1200),"
                 "engine_capacity decimal(3,1) not null, check(engine_capacity between 0.5 and 12.0),  "
                 "price bigint not null,"
                 "status text DEFAULT 'on_sale')");
    //PQexec(c, "delete from autod where produced = 1900");
    //res = PQexec(c, "alter table autod alter column model set not null");
    if(PQresultStatus(res) != PGRES_TUPLES_OK){
        printf("%s", PQresultErrorMessage(res));
        return;
    }
    PQclear(res);
}

void lootabel_omanikud(PGconn *c){
    PGresult *res;
    res = PQexec(c, "Create table if not exists omanikud"
                 "(id serial primary key,"
                 "name text,"
                 "city text,"
                 "country text,"
                 "unique(name))");
    if(PQresultStatus(res) != PGRES_TUPLES_OK){
        printf("%s", PQresultErrorMessage(res));
        return;
    }
    PQclear(res);
}
void lootabel_omanikud_ajalugu(PGconn *c){
    PGresult *res;
    res = PQexec(c, "Create table if not exists ajalugu("
                 "id serial PRIMARY KEY, "
                 "car_id int not null,"
                 "owner_id int not null,"
                 "start timestamp not null, "
                 "stop timestamp default NULL, check(start < stop))");

    if(PQresultStatus(res) != PGRES_TUPLES_OK){
        printf("%s", PQresultErrorMessage(res));
        return;
    }
    PQclear(res);
}

char *readText(char *name){
	ENTER(name);
	char buf[MAX];
	fgets(buf, MAX, stdin);

	while(strlen(buf)>0 && (buf[strlen(buf) - 1] == '\n' || buf[strlen(buf) - 1] == '\r' || buf[strlen(buf) - 1] == ' '))
		buf[strlen(buf) - 1] = '\0';

	char *val = malloc(strlen(buf) + 1);
	strcpy(val, buf);
	return val;
}
char *readInt(char *name){
	ENTER(name);
	char buf[MAX];
	fgets(buf, MAX, stdin);

	int x = atoi(buf);
	memset(buf, 0, MAX);
	sprintf(buf, "%d", x);

	char *val = malloc(strlen(buf) + 1);
	strcpy(val, buf);
	return val;
}

char *readNum(char *name){
	ENTER(name);
	char buf[MAX];
	fgets(buf, MAX, stdin);

    float x = atof(buf);
	memset(buf, 0, MAX);
	sprintf(buf, "%f", x);

	char *val = malloc(strlen(buf) + 1);
	strcpy(val, buf);
	return val;
}
PGresult *update(PGconn *c, char *query, int parcount, const char **params){
	PGresult *res;
	res = PQexecParams(c, query, parcount, NULL, params, NULL, NULL, 0);
	if(PQresultStatus(res) == PGRES_FATAL_ERROR) {
		printf("Failure %s\n", PQresultErrorMessage(res));
		return res;
	}
	PQclear(res);
	printf("***\n");
	return res;
}

int display(PGconn *connection, char *query, int parcount, const char **params){
    PGresult *res;//
    res = PQexecParams(connection, query, parcount, NULL, params, NULL, NULL, 0);

    if(PQresultStatus(res) != PGRES_TUPLES_OK){
        printf("Faulty query %s", PQresultErrorMessage(res));
        return 0;
    }
    int row, col;
    int row_count = PQntuples(res);
    int col_count = PQnfields(res);
    if (row_count < 1){
        printf("This table is empty\n");
        return 0;
    }

    for(col = 0; col < col_count; col++)
		printf("%20s | ", PQfname(res, col));
	printf("\n");

	for(row = 0; row < row_count; row++){
		for(col = 0; col < col_count; col++)
			printf("%20s | ", PQgetvalue(res, row, col));
		printf("\n");
	}
	PQclear(res);
	return 1;
}

void printmenu(){
    printf("\n--== Used car store ==--\n"
            "1: Display all tables\n"
            "2: Add car brand \n"
            "3: Add new car\n"
            "4: Add new owner\n"
            "5: Add car history\n"
            "6: Change price of a car\n"
            "7: Search by model and year, sort by price\n"
            "8: Search by engine_capacity\n"
            "9: Sort car brands by average price\n"
            "A: See how many cars on sale in each city\n"
            "H: Menu\n"
            "X: exit\n");
}

#include <stdio.h>
#include <mysql.h>
#include <string.h>
#include <stdlib.h>
#include <bitset>

#define max_categories 64
using namespace std;

MYSQL_RES *result;
MYSQL_ROW row;
MYSQL *connection, mysql;
int state;
const char * col_category="category";
const char * main_table ="main";

bitset<max_categories> binary_to_bitset(char * val)
{
	//printf("%s\n",val);
	unsigned long long ll;
	sscanf(val,"%llu",&ll);
	//printf("%llu\n",ll);
	unsigned long l1,l2;
	l1=ll;
	for (int i=0; i<max_categories/2;i++)
		l2=ll/2;
	
	//printf("%lu,%lu\n",l1,l2);
	bitset<max_categories/2> b1(l1);
	bitset<max_categories/2> b2(l1);
	char * bitstring=(char *)malloc(64*sizeof(char));
	sprintf( bitstring, "%s%s",b1.to_string().data(),b2.to_string().data());
	string temp=bitstring;
	bitset<max_categories> b(temp);
	
	//printf("%s\n",b.to_string().data());
	return b;
}
int start_connection()
{
	mysql_init(&mysql);
	connection = mysql_real_connect(&mysql,"localhost","root","","atm",0,0,0);

	if (connection == NULL) 
	{ 
		printf(mysql_error(&mysql)); 
		return 1; 
	}
	return 0;
}

void close_connection()
{
	mysql_free_result(result);
	mysql_close(connection); 
}

int insert_category(char * name)
{
	char * stmt=(char *)malloc(128*sizeof(char));;
	sprintf(stmt, "INSERT INTO categories(name) VALUES('%s')", name);
	state = mysql_query(connection, stmt);
	if (state !=0)
	{
		printf("%s\n",mysql_error(connection));
		return 1;
	}
	free(stmt);
	return 0;
}

int insert_word(char * ar_key, char * tr_key, char * stem, bitset<max_categories> category, int source_id, char * raw_form)
{
	char * stmt=(char *)malloc(1024*sizeof(char));;
	sprintf(stmt, "INSERT INTO main(ar_key, tr_key, stem, category, source_id, raw_form)  VALUES('%s','%s','%s',b'%s',%d,'%s')", ar_key, tr_key, stem, category.to_string().data(), source_id, raw_form);
	state = mysql_query(connection, stmt);
	if (state !=0)
	{
		printf("%s\n",mysql_error(connection));
		return 1;
	}
	free(stmt);
	return 0;
}

int display_table(char * table)
{
	
	char * stmt=(char *)malloc(128*sizeof(char));;
	sprintf(stmt, "SHOW COLUMNS FROM %s", table);
	state = mysql_query(connection, stmt);
	if (state !=0)
	{
		printf("%s\n",mysql_error(connection));
		return 1;
	}
	result = mysql_store_result(connection);

	int category_column=0;
	while ( ( row=mysql_fetch_row(result)) != NULL )
	{
		category_column--;
		printf("%s\t", (row[0] ? row[0] : "NULL"));
		if (strcmp((char*)row[0],col_category)==0)
			category_column=(-1)*category_column;
			
	}
	printf("\n");
	/*if (category_column>=0)
		sprintf(stmt, "SELECT id, ar_key, tr_key, stem, cast(category as unsigned), source_id,raw_form FROM %s", table);
	else*/
	sprintf(stmt, "SELECT * FROM %s", table);
	state = mysql_query(connection, stmt);
	
	if (state !=0)
	{
		printf("%s\n",mysql_error(connection));
		return 1;
	}

	result = mysql_store_result(connection);
	int columns=mysql_num_fields(result);
	while ( ( row=mysql_fetch_row(result)) != NULL )
	{
		for (int i=0; i< columns;i++)
		{
			if (i==4 && category_column>0)
			{
				bitset<max_categories> b=binary_to_bitset((char*)(row[i]));
				if (b.count()==0)
					printf("--------");
				else
					for (int k=0; k<max_categories;k++)
						if (b[k]==1)
							printf("%d,",k);
				printf("\t");
				//printf("<%s>\t", (row[i] ? row[i] : "NULL"));
			}
			else
				printf("%s\t", (row[i] ? row[i] : "NULL"));
		}
		printf("\n");
	}
	printf("Rows:%d\n",(int)mysql_num_rows(result));
	free(stmt);
	return 0;
}

bool test()
{
	char * stmt=(char *)malloc(128*sizeof(char));
	
	sprintf(stmt, "SELECT cast(category as unsigned) FROM main");
	state = mysql_query(connection, stmt);
	
	if (state !=0)
	{
		printf("%s\n",mysql_error(connection));
		return 1;
	}

	result = mysql_store_result(connection);
	while ( ( row=mysql_fetch_row(result)) != NULL )
	{
		bitset<max_categories> b=binary_to_bitset((char*)row[0]);
		for (int i=0; i<max_categories;i++)
			if (b[i]==1)
				printf("%d,",i);
	}
	free(stmt);
	return 0;
}


int main(int a,char **args)
{
	start_connection();
	//insert_category(args[1]);
	display_table("sources");
	display_table("categories");
	//bitset<max_categories> cat;
	//cat.set();
	//insert_word(args[1], "", "", cat,1,"");
	display_table("main");
	//test();
	close_connection();
	return 0;
};

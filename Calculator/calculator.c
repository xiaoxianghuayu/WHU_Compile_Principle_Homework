// caculation.c
#include <stdio.h>
#include <stdlib.h>
#define MAX_VAR_NAME 20
#define MAX_TOKENS 200
#define MAX_SYMBOL_TABLE 200
#define MAX_LINE 200
#define MAX_WORD 20
#define MAX_KEY 3
#define MAX_STACK 200
#define assert(p) if(!(p)) {printf("Assertion failure on line %d\n", __LINE__);exit(1);}

enum Type { 
	INT, 	// 整数 0
	FLOAT, 	// 浮点数 1
	SIGN, 	// 运算符 2
	X, 		// 自定义变量 3
	DEF_I, 	// int a的int 4
	DEF_F, 	// float a的float 5
	CALL,	// 调用函数 6
	ASSIGN, // = 7
	END, 	// . 8
	SEM,  	// ; 9
	KEY,	// 关键字 10
	CONS,  	// 常量 11
	COMA 	// , 12
};
char key_word[MAX_KEY][MAX_WORD] = {"float", "int", "write"};

typedef struct _Token { 		// name : 变量名, type是种类
	char name[MAX_VAR_NAME];
	int type = -1;
} Token;
typedef struct _Symbol {
	char name[MAX_VAR_NAME];
	int id; 					// 识别id
	bool assigned = 0; 			// 默认全部没有赋值
	bool used = 0; 				// 标识该变量是否有用, 默认无用 - 可填充
	int type;
	union _value {
		int i_value;
		float f_value;
	} value;
} Symbol;

Symbol symbol_table[MAX_SYMBOL_TABLE];
int symbol_count = 0;

bool is_digit(char c);
bool is_letter(char c);
int str_to_int(char *word);
float str_to_float(char *word);
bool is_div(char c); 										// 是否是分隔符
int Into_Tokens(char* line, Token *tokens, int line_num);  	// 把代码变成tokens
int Valid_Tokens(int num, Token *tokens, int line_num); 	// 把tokens中的定义部分写入符号表, 判断是否有错误
int Find_Table(Token token); 								// 遍历符号表, 寻找是否有与参数相同的符号
int Reg_Table(Token token, int i, Token tool_token); 		// 注册符号表
int Run_Tokens(Token *tokens, int num, int line_num);		// 计算赋值输出, 实际运行tokens
float calculate(char *expression, int line_num, int return_type);	// 开始计算
float expression_to_ans(char *expression, int *flag, int line_num);	// 转换成逆波兰表达式
float rpn_to_float(char *postexp, int *flag, int line_num);			// 逆波兰表达式求值
void error(char *text, int line_num, char *name);
int strlen(char *s);
int strcmp(char *a, char *b);
char* strcpy(char *a, char *b);
char* strncpy(char *a, char *b, int n);

int main(int argc, char const *argv[])
{
	int line_num = 0;
	char line[MAX_LINE];
	FILE *fp;
	Token tokens[MAX_TOKENS];
	if((fp = fopen(argv[1], "rt")) == NULL) {
		printf("Could not open %s!", argv[1]);
		return 1;
	}
	while(fgets(line, MAX_LINE, fp) != NULL) { // 读取到EOF返回NULL
		line_num ++;
		int num = Into_Tokens(line, tokens, line_num); 	// 每行处理一次, 如果是完整的, 包括各种结构就得全部合成到一起然后处理 ,并没有检查合法性
		if(num - 1 >= 0 && tokens[num - 1].type != SEM && tokens[num - 1].type != END) {
			error("Lack of ';'", line_num, 0);
		}
		Valid_Tokens(num, tokens, line_num);
		// 到这儿已经检查完毕所有的Token, 假设代码无误, 开始计算
		Run_Tokens(tokens, num, line_num);
	}
	
	return 0;
}

bool is_digit(char c) {
	return '0' <= c && c <= '9';
}

bool is_letter(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool is_sign(char c) {
	return (c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')');
}

int str_to_int(char *word) {
	int tmp = 0;
	for(int i = 0; i < strlen(word); i ++) {
		tmp = (word[i] - '0') + tmp * 10;
	}
	return tmp;
}

float str_to_float(char *word) {
	float tmp = 0;
	int times = 1;
	bool after_comma = 0;
	for(int i = 0; i < strlen(word); i++) {
		if(word[i] == '.') {
			after_comma = 1;
		} else {
			tmp = (word[i] - '0') + tmp * 10;
		}
		if(after_comma) {
			times *= 10;
		}
		if(times >= 1000000) { // 只保留6位 
			break;
		}
	}
	if(times != 1)
		times /= 10;
	return tmp / times;
}

bool is_div(char c) {
	return (c == ' ' || c == '\n' || c == ';' || c == '\t' || c == ',');
}

bool is_keyword(char *word) {
	for(int i = 0; i < MAX_KEY; i++) {
		if(!strcmp(word, key_word[i])) {
			return i + 1;
		}
	}
	return 0;
}

int Into_Tokens(char* line, Token* tokens, int line_num) {
	int count = 0;
	char tmp;
	char word[MAX_WORD];
	char *w = word;
	int i = 0;
	while (i < strlen(line)) {
		tmp = line[i];
		if(line[i] == '/' && i + 1 < strlen(line) && line[i+1] == '/') {
			return count;
		}
		if(is_letter(tmp) || is_digit(tmp) || tmp == '.') {
			(*w) = tmp;
			w++;
			i++;
			if(i == strlen(line)) {
				strcpy(tokens[count].name, ".");
				tokens[count].type = END;
				return count + 1;
			}
		} else { 			// 当前面的一串字符串读完碰到非数字/字母时就会进入
			(*w) = '\0';  	// 每个word后面截断, 方便字符串处理
			w = word;
			assert(word == w);

			int suc = 0;
			for(int j = 0; j < MAX_KEY; j++) { 		// 判断关键字
				if(!strcmp(word, key_word[j])) { 	// float int write
					switch(j) {
						case 0: tokens[count].type = DEF_F; break;
						case 1: tokens[count].type = DEF_I; break;
						case 2: tokens[count].type = CALL; break;
					}
					suc = 1;
				}
			}

			int flag = 1;     // 整型
			for(int k = 0; k < strlen(word); k++) {
				if(!is_digit(word[k])) {
					flag = 0; // 变量名
				}
				if(word[k] == '.') {
					if(flag == 2) {
						error("Synatx error", line_num, 0);
					}
					flag = 2; // 浮点数
				}
			}
			if(flag == 1) {
				tokens[count].type = INT;
			} else if (flag == 2) {
				tokens[count].type = FLOAT;
			} else if (flag == 0 && suc == 0) {
				tokens[count].type = X; 			// 如果是自定义变量就初始化符号表
			} else {
				if(!suc)
					error("Synatx error", line_num, 0);
			}

			if(tokens[count].type != -1 && strlen(word) > 0) {
				strcpy(tokens[count].name, word); 	// 赋值名字
				count++;
			}
			if(is_div(tmp)) {
				i++;
				if(tmp == ';') {
					strcpy(tokens[count].name, ";");
					tokens[count].type = SEM;
					count++;
				} else if(tmp == ',') {
					strcpy(tokens[count].name, ",");
					tokens[count].type = COMA;
					count++;
				}
			} else if(is_sign(tmp) || tmp == '=') {
				char t1[2];
				t1[0] = tmp;
				t1[1] = '\0';
				strcpy(tokens[count].name, t1);
				tokens[count].type = tmp == '=' ? ASSIGN : SIGN;
				i++;
				count++;             
			} else {
				error("Unexpected '%c' error", line_num, &tmp);
			}
		}

	}
	return count;
}

int Find_Table(Token token) {
	for(int i = 0; i < symbol_count; i++) {
		if(!strcmp(symbol_table[i].name, token.name))
			return i;
	}
	return -1;
}

int Reg_Table(Token token, int i, Token tool_token) {
	symbol_table[symbol_count].id = symbol_count;
	symbol_table[symbol_count].used = 1;
	strcpy(symbol_table[symbol_count].name, token.name);
	symbol_table[symbol_count].assigned = 0;
	symbol_table[symbol_count].type = tool_token.type == DEF_I ? INT : FLOAT;
	symbol_count++;
	return 1;
}

int Valid_Tokens(int num, Token *tokens, int line_num) {
	bool will_def = 0;
	int type = -1;
	int continue_define = 1;
	for(int i = 0; i < num; i++) {
		type = tokens[i].type;
		if(type == DEF_I || type == DEF_F) {
			if(will_def == 1) {
				error("Two or more data types in declaration of variable", line_num, 0);
			}
			will_def = 1; 						// 即将定义一个变量
		} else if (type == X) { 				// 读取到变量, 若不存在需要注册
			int flag = Find_Table(tokens[i]);
			if(flag == -1 && will_def == 1) { 	// 不存在
				if(!is_letter(tokens[i].name[0])) {
					error("Variable could only be started by letters or _", line_num, 0);
				}
				Reg_Table(tokens[i], i, tokens[i - continue_define]);
				will_def = 0;
				if(i + 1 < num && !strcmp(tokens[i + 1].name, ",")) {
					will_def = 1;
					i += 1;
					continue_define += 2;
				}
			} else if(flag != -1 && will_def == 1) { // 重复定义
				error("Variable %s could not be defined twice", line_num, tokens[i].name);
			} else if(flag == -1) {
				error("Undeclared variable %s", line_num, tokens[i].name);
			}
		} // 上述工作是为了定义变量并注册到符号表中

	}
	return 1;
}

int Run_Tokens(Token *tokens, int num, int line_num) {
	int i = 0;
	while(i < num) {
		while(tokens[i].type == DEF_F || tokens[i].type == DEF_I) {
			while(tokens[i].type != SEM) {
				i++;
			}
			i++;
		}
		if(i >= num)
			return 1;   // 定义已经进入符号表, 无需再次定义, 全部跳过
		if(i + 1 < num && tokens[i + 1].type == ASSIGN) {
			Token target;
			char expression[MAX_LINE];
			int c1 = 0;
			target = tokens[i];
			assert(tokens[i + 1].type == ASSIGN);
			i = i + 2;
			while(tokens[i].type != SEM && i < num) {
				strncpy(expression + c1, tokens[i].name, strlen(tokens[i].name)); // 根据后面读取数据方便做适当调整
				c1 = c1 + strlen(tokens[i].name);
				i++;
			}
			expression[c1] = '\0';
			int tmp = Find_Table(target);
			if(symbol_table[tmp].type == INT) {
				int tmp_int = (int)calculate(expression, line_num, 0);
				// 判断, 类型转换
				symbol_table[tmp].value.i_value = tmp_int;
				symbol_table[tmp].assigned = 1;
			} else if(symbol_table[tmp].type == FLOAT) {
				float tmp_float = calculate(expression, line_num, 1);
				// 判断, 类型转换
				symbol_table[tmp].value.f_value = tmp_float;
				symbol_table[tmp].assigned = 1;
			} else {
				error("Unexpected error1", line_num, 0);
			}
		} else if(tokens[i].type == CALL) {
			// Write函数
			i = i + 2;
			Token tmp_token;
			strcpy(tmp_token.name, tokens[i].name);
			int k = Find_Table(tmp_token);
			if(k == -1) {
				error("The arg of write could not be empty!", line_num, 0);
			}
			int tmp_type = symbol_table[k].type;
			if(tmp_type == INT) {
				printf("%d\n", symbol_table[k].value.i_value);
			} else if(tmp_type == FLOAT) {
				printf("%.2f\n", symbol_table[k].value.f_value);
			} else {
				error("Unexpected error2", line_num, 0);
			}
		} else {
		}
		i++;
	}
	return 1;
}

float calculate(char *expression, int line_num, int return_type) {
	int flag = 0; // 返回值: flag = 0 整数, flag = 1 浮点数, 由于从符号表中取出时才能判断, 所以需要一直传参
	float ans = 0;
	char tmp;
	// 处理表达式, 都用符号带入
	ans = expression_to_ans(expression, &flag, line_num);
	if(return_type == 1 && flag == 0) {
		error("Could change `int` to `float`", line_num, 0);
	}
	//计算表达式并返回
	return flag == 0 ? (int)ans : ans;
}

float expression_to_ans(char *expression, int *flag, int line_num) {
	char postexp[1000];  	// 后缀表达式的最大长度
	int count = 0; 			// 后缀表达式的指针
	char st[MAX_STACK];  	// 当做栈用
	int num = 0; 			// 栈的栈顶指针
	int minus = 0;			// 标识即将处理负数
	int in_once = 0;		// -前面只有一个括号就执行
	char e;
	char *exp = expression;
	while(*exp != '\0') {
		switch(*exp) {
			case '(': 
				st[num++] = '('; 	// PUSH
				exp++; 
				break;
			case ')': 
				e = st[--num];   	// POP
				while(e != '(') {
					if(num == 0) {
						error("expected ';' before ')' token", line_num, 0);
					}
					postexp[count++] = e;
					e = st[--num];
				}
				exp++;
				break;
			case '+':
			case '-':
				if(num == 0 && count == 0) { // 用于-1 * 2这种情况
					postexp[count++] = '0';
					postexp[count++] = '#';
					minus = 1;
					exp++;
				}
				while(num != 0) {		// !Stack Empty
					e = st[num - 1];	// Get Top
					in_once += 1;
					if(e != '(') {
						postexp[count++] = e;
						e = st[--num];
					} else {				// 用于(-1)*2 和(-1 * 2)
						if(in_once == 1) {
							postexp[count++] = '0';
							postexp[count++] = '#';
							minus = 1;
							exp++;
						}
						break;
					}
				}
				if(minus == 0) {
					st[num++] = *exp;
					exp++;
				}
				break;
			case '*':
			case '/':
				while(num != 0) {
					e = st[num - 1];
					if(e == '*' || e == '/') {
						postexp[count++] = e;
						e = st[--num];
					} else {
						break;
					}
				}
				st[num++] = *exp;
				exp++;
				break;
			default: // 处理数据
				while(is_letter(*exp) || is_digit(*exp) || *exp == '.') { //变量允许的字符
					postexp[count++] = *exp;
					exp++;
				}
				postexp[count++] = '#';
				if(minus == 1) {
					postexp[count++] = '-';
					minus = 0;
				}
				break;
		}
	}

	while(num != 0) {
		e = st[--num];
		postexp[count++] = e;
	}
	postexp[count] = '\0';
	float ans = rpn_to_float(postexp, flag, line_num);
	return ans;
}

float rpn_to_float(char *postexp, int *flag, int line_num) {
	float a,b,c,d,e; 	
	float st[MAX_STACK];
	int num = 0;
	while(*postexp != '\0') { 
		switch(*postexp) {
			case '+':
				a = st[--num];
				b = st[--num];
				c = a + b;
				st[num++] = c;
				break;
			case '-':
				a = st[--num];
				b = st[--num];
				c = b - a;
				st[num++] = c;
				break;
			case '*':
				a = st[--num];
				b = st[--num];
				c = a * b;
				st[num++] = c;
				break;
			case '/':
				a = st[--num];
				b = st[--num];
				if(a == 0) {
					error("Div zero error!", line_num, 0);
				}
				c = b / a;
				st[num++] = c;
				break;
			case '#':
				break;
			default:
				Token tmp;
				int t1 = 0;
				while(is_letter(*postexp) || is_digit(*postexp) || *postexp == '.') {
					tmp.name[t1++] = *postexp;
					postexp++;
				}
				tmp.name[t1] = '\0';

				// 判断纯数字
				bool is_immediate = 1;  
				for(int i = 0; i < strlen(tmp.name); i++) {
					if(tmp.name[i] == '.')
						*flag = 1;
					if(!(is_digit(tmp.name[i]) || tmp.name[i] == '.')) {
						is_immediate = 0;
					}
				}
				if(is_immediate) {
					st[num++] = str_to_float(tmp.name);
					break;
				}

				int k = Find_Table(tmp);
				if(symbol_table[k].assigned != 1) {
					error("Could use variable '%s' that have not been assigned!", line_num, tmp.name);
				}
				if(symbol_table[k].type == FLOAT) {
					*flag = 1;
				}
				st[num++] = symbol_table[k].type == INT ? (float)symbol_table[k].value.i_value : symbol_table[k].value.f_value;
				break;
		}
		postexp++;
	}
	e = st[num - 1];
	return e;
}

void error(char *text, int line_num, char *name) {
	assert(line_num != 0);
	char buffer[1000];
	printf("%s: ", __FILE__);
	sprintf(buffer, ":%d: ERROR:\n\t", line_num);
	printf("%s", buffer);
	sprintf(buffer, text, name);
	printf("%s\n", buffer);
	exit(1);
}

int strlen(char *s) {
	int count = 0;
	while(*s != '\0') {
		count++;
		s++;
	}
	return count;
}

int strcmp(char *a, char *b) {
	while(*a == *b) {
		assert((a != NULL) && (b != NULL));
		if(*a == '\0')
			return 0;
		a++;
		b++;
	}
	return *a - *b;
}

char *strcpy(char *a, char *b) {
	char *r = a;
	assert((a != NULL) && (b != NULL));
	while((*a++ = *b++) != '\0');
	return r;
}

char *strncpy(char *a, char *b, int n) {
	char *r = a;
	int count = 0;
	assert((a != NULL) && (b != NULL));
	while((*a++ = *b++) != '\0' && count++ < n);
	return r;
}

# КС-грамматика языка

**1. Структура программы и операторы**
```text
A → Q A | λ
Q → D | E | C | L | I | O
```

**2. Объявление переменных**
```text
D → Y id M R ;
Y → int | float
M → [ num_int ] | λ
R → , id M R | λ
```

**3. Оператор присваивания**
```text
E → id idx = S ;
idx → [ S ] | λ
```

**4. Арифметические выражения**
```text
S → T U
U → + T U | - T U | λ
T → F W
W → * F W | / F W | λ
F → ( S ) | id idx | num_int | num_float | - F | Fn ( S )
Fn → sqrt | exp | log
```

**5. Условные конструкции и циклы**
```text
C → if ( V ) { A } B
B → else { A } | λ
L → while ( V ) { A }
```

**6. Логические условия**
```text
V → S K S
K → == | != | < | > | <= | >=
```

**7. Ввод и Вывод**
```text
I → input ( id idx ) ;
O → print ( S ) ;
```

---

# КС-грамматика языка в нестрогой нормальной форме Грейбах

В этой форме все правые части начинаются с терминала (или пусты). Поскольку нетерминалы `S`, `T`, `V` начинают вывод через `F`, в каждом из них дублируются все начальные терминалы из `F` — включая `sqrt`, `exp`, `log`.

```text
A → Q ; A | λ

Q → int ID | float ID | ID = S | ID [ S ] = S 
  | if ( V ) { A } B 
  | while ( V ) { A } 
  | input ID 
  | print ( S )

S → ( S ) U | ID U | ID [ S ] U 
  | NUM_INT U | NUM_FLOAT U 
  | - F U 
  | sqrt ( S ) U | exp ( S ) U | log ( S ) U

U → + T U | - T U | λ

T → ( S ) W | ID W | ID [ S ] W 
  | NUM_INT W | NUM_FLOAT W 
  | - F W 
  | sqrt ( S ) W | exp ( S ) W | log ( S ) W

W → * F W | / F W | λ

F → ( S ) | ID | ID [ S ] 
  | NUM_INT | NUM_FLOAT 
  | - F 
  | sqrt ( S ) | exp ( S ) | log ( S )

B → else { A } | λ

V → ( S ) U K S | ID U K S | ID [ S ] U K S 
  | NUM_INT U K S | NUM_FLOAT U K S 
  | - F U K S 
  | sqrt ( S ) U K S | exp ( S ) U K S | log ( S ) U K S

K → == | != | < | > | <= | >=
```

> **Замечание о LL(1).** Терминалы `sqrt`, `exp`, `log` различны между собой и не пересекаются с другими терминалами в правых частях, поэтому таблица LL(1)-анализатора однозначна — конфликтов не возникает.
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
F → ( S ) | id idx | num_int | num_float | - F
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

```text
A → Q ; A | λ

Q → int ID | float ID | ID = S | ID [ S ] = S | if ( V ) { A } B | while ( V ) { A } | input ID | print ( S )

S → ( S ) U | ID U | ID [ S ] U | NUM_INT U | NUM_FLOAT U | - F U

U → + T U | - T U | λ

T → ( S ) W | ID W | ID [ S ] W | NUM_INT W | NUM_FLOAT W | - F W

W → * F W | / F W | λ

F → ( S ) | ID | ID [ S ] | NUM_INT | NUM_FLOAT | - F

B → else { A } | λ

V → ( S ) U K S | ID U K S | ID [ S ] U K S | NUM_INT U K S | NUM_FLOAT U K S | - F U K S

K → == | != | < | > | <= | >=
```
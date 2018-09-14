header = ['id', 'Name', 'Type', 'Cost', 'Damage', 'Health', 'Abilities', 'PlayerHP',
          'EnemyHP', 'CardDraw', 'Text description']
data = []

while True:
    try:
        line = input()
    except EOFError:
        break
    record = [token.strip() for token in line.split(';')]
    data.append(record)

data.sort(key = lambda r: int(r[3]))

print(header)

cost = []
attackPower = []

for record in data:
    if record[6].count('G') > 0:
        cost.append(record[3])
        attackPower.append(record[4])

print(list(zip(cost, attackPower)))
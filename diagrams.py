import pandas as pd
import matplotlib.pyplot as plt

#Διάβασμα των δεδομένων από το CSV που έφτιαξε η C
try:
    df = pd.read_csv('wait_times.csv')
except FileNotFoundError:
    print("Το αρχείο 'wait_times.csv' δεν βρέθηκε. Τρέξε πρώτα το πρόγραμμα σε C.")
    exit()

plt.figure(figsize=(12, 6))

#Διάγραμμα 2: Ιστόγραμμα Κατανομής
plt.hist(df['Wait_Time_us'], bins=50, color='blue', edgecolor='black', alpha=0.7)
plt.title('Κατανομή Χρόνων Αναμονής')
plt.xlabel('Χρόνος Αναμονής (μs)')
plt.ylabel('Συχνότητα (Πλήθος Tasks)')
plt.grid(True, axis='y', linestyle='--', alpha=0.6)

plt.tight_layout()
plt.show()
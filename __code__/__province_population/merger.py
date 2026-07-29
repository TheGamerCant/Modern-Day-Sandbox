import pandas as pd

# Input files
table_a = "vp_populations.csv"
table_b = "vp_populations_2.csv"

# Output file
output = "merged.csv"

# Read CSVs
df_a = pd.read_csv(table_a, dtype=str, encoding="utf-8-sig").fillna("")
df_b = pd.read_csv(table_b, dtype=str, encoding="utf-8-sig").fillna("")

# Key columns
keys = ["prov_id", "names", "owner"]

# The last two columns to overwrite
value_columns = list(df_a.columns[-2:])

# Merge
merged = df_a.merge(
    df_b[keys + value_columns],
    on=keys,
    how="left",
    suffixes=("", "_b")
)

# Overwrite only when Table B's value is not empty
for col in value_columns:
    merged[col] = merged[f"{col}_b"].where(
        merged[f"{col}_b"] != "",
        merged[col]
    )
    merged.drop(columns=f"{col}_b", inplace=True)

# Save
merged.to_csv(output, index=False, encoding="utf-8-sig")

print(f"Saved merged file to {output}")
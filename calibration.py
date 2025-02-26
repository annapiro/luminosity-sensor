import csv

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy.optimize import curve_fit


# Define the model - a power function is the appropriate mathematical relationship according to
# Feng & Wang (2020). Design and development of a low-cost glazing measurement system.
def model_function(x, a, b):
    return a * (x ** b)


df_am0 = pd.read_csv('data/AM0_grouped.csv')
df_am15g = pd.read_csv('data/AM15G_grouped.csv')

# construct a dictionary with reference irradiance values for different spectra
spectrum_reference = dict()
with open('data/spectrum_reference.csv', 'r') as csvfile:
    reader = csv.reader(csvfile)
    next(reader)  # skip headers
    for line in reader:
        spectrum, intensity, irradiance = line
        if spectrum not in spectrum_reference:
            spectrum_reference[spectrum] = dict()
        spectrum_reference[spectrum][int(intensity)] = float(irradiance)

# correlate the sensor's raw counts with target irradiance values
xy_data_am0 = pd.DataFrame({
    'Counts': df_am0['CH0'],
    'Irradiance': df_am0['Intensity'].apply(lambda x: spectrum_reference['AM0'].get(x))
})

xy_data_am15g = pd.DataFrame({
    'Counts': df_am15g['CH0'],
    'Irradiance': df_am15g['Intensity'].apply(lambda x: spectrum_reference['AM1.5G'].get(x))
})

# combine the measurements from both spectra into a single training dataset
xy_data = pd.concat([xy_data_am0, xy_data_am15g], ignore_index=True)
xy_data.sort_values('Counts', inplace=True, ignore_index=True)

X = xy_data['Counts']
Y = xy_data['Irradiance']

# fit the model
params, _ = curve_fit(model_function, X, Y)
y_fit = model_function(X, *params)

# extract the coefficients
a, b = params
print(f"Fitted coefficients: a = {a}, b = {b}")

# plot the results
plt.plot(X, y_fit, color='red', label='Fitted curve', zorder=1)
plt.scatter(X, Y, color='blue', label='Measurements', s=5, zorder=2)
plt.xlabel('Raw counts')
plt.ylabel('Irradiance [W/m²]')
plt.title('Fitting results (y = axᵇ)')
plt.legend()
plt.savefig('fitted_results.png', dpi=150)
plt.close()

# calculate residuals
residuals = Y - y_fit
mae = np.mean(np.abs(residuals))
plt.scatter(X, residuals, s=5)
plt.axhline(0, color='red', linestyle='--')
plt.xlabel('Raw counts')
plt.ylabel('Residuals')
plt.title(f"Residuals of fitted curve (mean error: {mae:.2f})")
plt.savefig('residuals.png', dpi=150)
plt.close()

ss_res = np.sum(residuals**2)  # sum of squares of residuals
ss_tot = np.sum((Y - np.mean(Y))**2)  # total sum of squares
r_squared = 1 - (ss_res / ss_tot)
print(f"R-squared: {r_squared:.4f}")

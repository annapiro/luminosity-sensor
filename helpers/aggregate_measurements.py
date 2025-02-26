import pandas as pd

# Aggregate measurements based on intensity (as measured by the spectrometer in W/m2)
data = pd.read_csv('testing.csv')
# group by Intensity, aggregate AM15G by mode and Irradiance and Error by mean
grouped_data = data.groupby('Intensity(Spectrometer)[W/m2]').agg(AM15G=('AM1.5G', lambda x: x.mode()[0]),
                                                                 Irradiance=('Irradiance[W/m2]', 'mean'),
                                                                 Error=('Error[W/m2]', 'mean'))
# round the values to 2 decimal places
grouped_data[['Irradiance', 'Error']] = grouped_data[['Irradiance', 'Error']].round(2)
grouped_data.to_csv('grouped.csv')

# Aggregate measurements made within the same one-minute window
data = pd.read_csv('temp_test_xenon.csv')
# convert time values to specified datetime format ("16:25:55.701" to "1900-01-01 16:25:55.701")
data['Time'] = pd.to_datetime(data['Time'], format='%H:%M:%S.%f')
# check whether the difference between consecutive time entries is greater than 1 minute
# cumulative sum effectively creates a unique group id for each segment where the time delta is below threshold
data['Group'] = (data['Time'].diff() > pd.Timedelta(minutes=1)).cumsum()
# group based on that id, aggregate Time based on the first entry, other columns self-explanatory
grouped = data.groupby('Group').agg(Time=('Time', lambda x: x.iloc[0].strftime('%H:%M:%S')),
                                    CH0=('CH0', 'mean'),
                                    CH1=('CH1', 'mean'),
                                    Lux=('Lux', 'mean'),
                                    Irradiance=('Irradiance[W/m2]', 'mean'),
                                    Error=('Error[W/m2]', 'mean'))
# round to 2 decimal places
grouped[['CH0', 'CH1', 'Lux', 'Irradiance', 'Error']] = grouped[['CH0', 'CH1', 'Lux', 'Irradiance', 'Error']].round(2)
grouped.to_csv('grouped_xenon.csv')

# Differential Privacy for geolocation
## Purpose
This study proposes a novel approach for analyzing location-based information that incorporates report geolocation data. This method is designed to ensure compliance with privacy requirements, utilizing the local differential privacy methodology known as RAPPOR. The new methodology transmits location information generated via Local Differential Privacy (LDP). Subsequently, original geolocation data is reconstructed from perturbated data set. For this, we perform discretization of original geolocation data before perturbation. Overall flow of this methodology is below:<br /><hr />
__Objective:__ Sending geolocation data ensure privacy requirement. Even sending perturbed data, aggregator can recognize geolocation trend of whole data.<br /><br />
__Methodology:__ This research use RAPPOR to keep privacy for sending sensitive geolocation data. For reducing reconstruction consuming time of perturbated data, discretize coordinates by rounding latitude and longitude coordinates. When round coordinates accuracy of geolocation coordination will be changed. In this research, perform different conditions evaluation and find out most suitable numbers of decimal places.<br /><br />
__Data:__ We use US Accident data on Kaggle under common creative license<br /><br />
![US Accident mapping](https://github.com/mystous/differential-privacy-for-geolocation/blob/main/images/us_accident_mapping.png)
## RAPPOR Applied
![](https://github.com/mystous/differential-privacy-for-geolocation/blob/main/images/rappor_applied_sample.png)

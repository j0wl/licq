#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "countrycodes.h"

const struct SCountry gCountries[NUM_COUNTRIES + 1] =
{
  { "USA", 1, 0 },
  { "Russia", 7, 1 },
  { "Egypt", 20, 2 },
  { "South Africa", 27, 3 },
  { "Greece", 30, 4 },
  { "Netherlands", 31, 5 },
  { "Belgium", 32, 6 },
  { "France", 33, 7 },
  { "Spain", 34, 8 },
  { "Hungary", 36, 9 },
  { "Italy", 39, 10 },
  { "Romania", 40, 11 },
  { "Switzerland", 41, 12 },
  { "Czech Republic", 42, 13 },
  { "Austria", 43, 14 },
  { "United Kingdom", 44, 15 },
  { "Denmark", 45, 16 },
  { "Sweden", 46, 17 },
  { "Norway", 47, 18 },
  { "Poland", 48, 19 },
  { "Germany", 49, 20 },
  { "Peru", 51, 21 },
  { "Mexico", 52, 22 },
  { "Cuba", 53, 23 },
  { "Argentina", 54, 24 },
  { "Brazil", 55, 25 },
  { "Chile", 56, 26 },
  { "Columbia", 57, 27 },
  { "Venezuela", 58, 28 },
  { "Malaysia", 60, 29 },
  { "Australia", 61, 30 },
  { "Indonesia", 62, 31 },
  { "Philippines", 63, 32 },
  { "New Zealand", 64, 33 },
  { "Singapore", 65, 34 },
  { "Thailand", 66, 35 },
  { "Japan", 81, 36 },
  { "Korea (Republic of)", 82, 37 },
  { "Vietnam", 84, 38 },
  { "China", 86, 39 },
  { "Turkey", 90, 40 },
  { "India", 91, 41 },
  { "Pakistan", 92, 42 },
  { "Afghanistan", 93, 43 },
  { "Sri Lanka", 94, 44 },
  { "Myanmar", 95, 45 },
  { "Iran", 98, 46 },
  { "Anguilla", 101, 47 },
  { "Antigua", 102, 48 },
  { "Bahamas", 103, 49 },
  { "Barbados", 104, 50 },
  { "Bermuda", 105, 51 },
  { "British Virgin Islands", 106, 52 },
  { "Canada", 107, 53 },
  { "Cayman Islands", 108, 54 },
  { "Dominica", 109, 55 },
  { "Dominican Republic", 110, 56 },
  { "Grenada", 111, 57 },
  { "Jamaica", 112, 58 },
  { "Montserrat", 113, 59 },
  { "Nevis", 114, 60 },
  { "St. Kitts", 115, 61 },
  { "St. Vincent and the Grenadines", 116, 62 },
  { "Trinidad and Tobago", 117, 63 },
  { "Turks and Caicos Islands", 118, 64 },
  { "Barbuda", 120, 65 },
  { "Puerto Rico", 121, 66 },
  { "Saint Lucia", 122, 67 },
  { "United States Virgin Islands", 123, 68 },
  { "Morocco", 212, 69 },
  { "Algeria", 213, 70 },
  { "Tunisia", 216, 71 },
  { "Libya", 218, 72 },
  { "Gambia", 220, 73 },
  { "Senegal Republic", 221, 74 },
  { "Mauritania", 222, 75 },
  { "Mali", 223, 76 },
  { "Guinea", 224, 77 },
  { "Ivory Coast", 225, 78 },
  { "Burkina Faso", 226, 79 },
  { "Niger", 227, 80 },
  { "Togo", 228, 81 },
  { "Benin", 229, 82 },
  { "Mauritius", 230, 83 },
  { "Liberia", 231, 84 },
  { "Sierra Leone", 232, 85 },
  { "Ghana", 233, 86 },
  { "Nigeria", 234, 87 },
  { "Chad", 235, 88 },
  { "Central African Republic", 236, 89 },
  { "Cameroon", 237, 90 },
  { "Cape Verde Islands", 238, 91 },
  { "Sao Tome and Principe", 239, 92 },
  { "Equatorial Guinea", 240, 93 },
  { "Gabon", 241, 94 },
  { "Congo", 242, 95 },
  { "Zaire", 243, 96 },
  { "Angola", 244, 97 },
  { "Guinea-Bissau", 245, 98 },
  { "Diego Garcia", 246, 99 },
  { "Ascension Island", 247, 100 },
  { "Seychelle Islands", 248, 101 },
  { "Sudan", 249, 102 },
  { "Rwanda", 250, 103 },
  { "Ethiopia", 251, 104 },
  { "Somalia", 252, 105 },
  { "Djibouti", 253, 106 },
  { "Kenya", 254, 107 },
  { "Tanzania", 255, 108 },
  { "Uganda", 256, 109 },
  { "Burundi", 257, 110 },
  { "Mozambique", 258, 111 },
  { "Zambia", 260, 112 },
  { "Madagascar", 261, 113 },
  { "Reunion Island", 262, 114 },
  { "Zimbabwe", 263, 115 },
  { "Namibia", 264, 116 },
  { "Malawi", 265, 117 },
  { "Lesotho", 266, 118 },
  { "Botswana", 267, 119 },
  { "Swaziland", 268, 120 },
  { "Mayotte Island", 269, 121 },
  { "St. Helena", 290, 122 },
  { "Eritrea", 291, 123 },
  { "Aruba", 297, 124 },
  { "Faeroe Islands", 298, 125 },
  { "Greenland", 299, 126 },
  { "Gibraltar", 350, 127 },
  { "Portugal", 351, 128 },
  { "Luxembourg", 352, 129 },
  { "Ireland", 353, 130 },
  { "Iceland", 354, 131 },
  { "Albania", 355, 132 },
  { "Malta", 356, 133 },
  { "Cyprus", 357, 134 },
  { "Finland", 358, 135 },
  { "Bulgaria", 359, 136 },
  { "Lithuania", 370, 137 },
  { "Latvia", 371, 138 },
  { "Estonia", 372, 139 },
  { "Moldova", 373, 140 },
  { "Armenia", 374, 141 },
  { "Belarus", 375, 142 },
  { "Andorra", 376, 143 },
  { "Monaco", 377, 144 },
  { "San Marino", 378, 145 },
  { "Vatican City", 379, 146 },
  { "Ukraine", 380, 147 },
  { "Yugoslavia", 381, 148 },
  { "Croatia", 385, 149 },
  { "Slovenia", 386, 150 },
  { "Bosnia and Herzegovina", 387, 151 },
//  { "Former Yugoslav Republic of Macedonia", 389, 152 },
  { "Republic of Macedonia", 389, 152 },
  { "Falkland Islands", 500, 153 },
  { "Belize", 501, 154 },
  { "Guatemala", 502, 155 },
  { "El Salvador", 503, 156 },
  { "Honduras", 504, 157 },
  { "Nicaragua", 505, 158 },
  { "Costa Rica", 506, 159 },
  { "Panama", 507, 160 },
  { "St. Pierre and Miquelon", 508, 161 },
  { "Haiti", 509, 162 },
  { "Guadeloupe", 590, 163 },
  { "Bolivia", 591, 164 },
  { "Guyana", 592, 165 },
  { "Ecuador", 593, 166 },
  { "French Guiana", 594, 167 },
  { "Paraguay", 595, 168 },
  { "Martinique", 596, 169 },
  { "Suriname", 597, 170 },
  { "Uruguay", 598, 171 },
  { "Netherlands Antilles", 599, 172 },
  { "Saipan Island", 670, 173 },
  { "Guam", 671, 174 },
  { "Christmas Island", 672, 175 },
  { "Brunei", 673, 176 },
  { "Nauru", 674, 177 },
  { "Papua New Guinea", 675, 178 },
  { "Tonga", 676, 179 },
  { "Solomon Islands", 677, 180 },
  { "Vanuatu", 678, 181 },
  { "Fiji Islands", 679, 182 },
  { "Palau", 680, 183 },
  { "Wallis and Futuna Islands", 681, 184 },
  { "Cook Islands", 682, 185 },
  { "Niue", 683, 186 },
  { "American Samoa", 684, 187 },
  { "Western Samoa", 685, 188 },
  { "Kiribati Republic", 686, 189 },
  { "New Caledonia", 687, 190 },
  { "Tuvalu", 688, 191 },
  { "French Polynesia", 689, 192 },
  { "Tokelau", 690, 193 },
  { "Micronesia, Federated States of", 691, 194 },
  { "Marshall Islands", 692, 195 },
  { "Kazakhstan", 705, 196 },
  { "Kyrgyz Republic", 706, 197 },
  { "Tajikistan", 708, 198 },
  { "Turkmenistan", 709, 199 },
  { "Uzbekistan", 711, 200 },
  { "International Freephone Service", 800, 201 },
  { "Korea (North)", 850, 202 },
  { "Hong Kong", 852, 203 },
  { "Macau", 853, 204 },
  { "Cambodia", 855, 205 },
  { "Laos", 856, 206 },
  { "INMARSAT", 870, 207 },
  { "INMARSAT (Atlantic-East)", 871, 208 },
  { "INMARSAT (Pacific)", 872, 209 },
  { "INMARSAT (Indian)", 873, 210 },
  { "INMARSAT (Atlantic-West)", 874, 211 },
  { "Bangladesh", 880, 212 },
  { "Taiwan, Republic of China", 886, 213 },
  { "Maldives", 960, 214 },
  { "Lebanon", 961, 215 },
  { "Jordan", 962, 216 },
  { "Syria", 963, 217 },
  { "Iraq", 964, 218 },
  { "Kuwait", 965, 219 },
  { "Saudi Arabia", 966, 220 },
  { "Yemen", 967, 221 },
  { "Oman", 968, 222 },
  { "United Arab Emirates", 971, 223 },
  { "Israel", 972, 224 },
  { "Bahrain", 973, 225 },
  { "Qatar", 974, 226 },
  { "Bhutan", 975, 227 },
  { "Mongolia", 976, 228 },
  { "Nepal", 977, 229 },
  { "Azerbaijan", 994, 230 },
  { "Georgia", 995, 231 },
  { "Comoros", 2691, 232 },
  { "Liechtenstein", 4101, 233 },
  { "Slovak Republic", 4201, 234 },
  { "Guantanamo Bay", 5399, 235 },
  { "French Antilles", 5901, 236 },
  { "Cocos-Keeling Islands", 6101, 237 },
  { "Rota Island", 6701, 238 },
  { "Tinian Island", 6702, 239 },
  { "Australian Antarctic Territory", 6721, 240 },
  { "Norfolk Island", 6722, 241 },
  { "Unspecified", COUNTRY_UNSPECIFIED,  242 }
};


const struct SCountry *GetCountryByCode(unsigned short _nCountryCode)
{
   // do a simple linear search as there aren't too many countries
   unsigned short i = 0;
   while (i < NUM_COUNTRIES && gCountries[i].nCode != _nCountryCode) i++;
   if (i == NUM_COUNTRIES) return NULL;
   return &gCountries[i];
}

const struct SCountry *GetCountryByIndex(unsigned short _nIndex)
{
   if (_nIndex >= NUM_COUNTRIES) return NULL;
   return (&gCountries[_nIndex]);
}


% ThingSpeak MATLAB Analysis script for weather-station forecast metrics.
% Run this inside ThingSpeak > Apps > MATLAB Analysis.

% ----------------------
% Required configuration
% ----------------------
readChannelID = 0;          % TODO: set your channel ID (numeric)
readAPIKey    = "";         % TODO: set read API key (empty if public)
writeChannelID = 0;         % TODO: set target channel for computed metrics
writeAPIKey    = "";        % TODO: set write API key

% Normalize credentials for ThingSpeak runtime (expects char keys).
readAPIKey = char(strtrim(string(readAPIKey)));
writeAPIKey = char(strtrim(string(writeAPIKey)));

if readChannelID <= 0
    error('Set readChannelID to a valid numeric channel ID.');
end

if writeChannelID <= 0
    error('Set writeChannelID to a valid numeric channel ID.');
end

% Write safety controls
% If read/write channel IDs are the same and this is false, write is skipped.
% Keeping them separate is strongly recommended on free tier limits.
allowSameChannelWrite = false;

% Source channel field mapping (from weather-station channel):
% field1: temperature
% field2: humidity
% field3: rain_intensity
temperatureField = 1;
humidityField = 2;
rainIntensityField = 3;

% Number of most recent points to analyze
numPoints = 120;
minValidPoints = 6;

% ----------------------
% Read data from channel
% ----------------------
% Read required source fields in one request to reduce API call volume.
if isempty(readAPIKey)
    % Use this only for public channels.
    dataTable = thingSpeakRead(readChannelID, ...
        'Fields', [temperatureField, humidityField, rainIntensityField], ...
        'NumPoints', numPoints, ...
        'OutputFormat', 'table');
else
    dataTable = thingSpeakRead(readChannelID, ...
        'Fields', [temperatureField, humidityField, rainIntensityField], ...
        'NumPoints', numPoints, ...
        'OutputFormat', 'table', ...
        'ReadKey', readAPIKey);
end

temperatureVarCandidates = { ...
    sprintf('Field%d', temperatureField), ...
    sprintf('field%d', temperatureField), ...
    'temperature'};
humidityVarCandidates = { ...
    sprintf('Field%d', humidityField), ...
    sprintf('field%d', humidityField), ...
    'humidity'};
rainVarCandidates = { ...
    sprintf('Field%d', rainIntensityField), ...
    sprintf('field%d', rainIntensityField), ...
    'rain_intensity'};

tableVars = dataTable.Properties.VariableNames;
tempVar = '';
humVar = '';
rainVar = '';

for i = 1:numel(temperatureVarCandidates)
    if any(strcmpi(tableVars, temperatureVarCandidates{i}))
        tempVar = tableVars{find(strcmpi(tableVars, temperatureVarCandidates{i}), 1)};
        break;
    end
end
for i = 1:numel(humidityVarCandidates)
    if any(strcmpi(tableVars, humidityVarCandidates{i}))
        humVar = tableVars{find(strcmpi(tableVars, humidityVarCandidates{i}), 1)};
        break;
    end
end
for i = 1:numel(rainVarCandidates)
    if any(strcmpi(tableVars, rainVarCandidates{i}))
        rainVar = tableVars{find(strcmpi(tableVars, rainVarCandidates{i}), 1)};
        break;
    end
end

if ~isempty(tempVar) && ~isempty(humVar) && ~isempty(rainVar)
    temperature = str2double(string(dataTable.(tempVar)));
    humidity = str2double(string(dataTable.(humVar)));
    rainIntensity = str2double(string(dataTable.(rainVar)));
else
    % Fallback path: re-read as numeric matrix and trust field order requested.
    fprintf('Falling back to matrix parse due to unexpected table column names.\n');
    if isempty(readAPIKey)
        matrixData = thingSpeakRead(readChannelID, ...
            'Fields', [temperatureField, humidityField, rainIntensityField], ...
            'NumPoints', numPoints);
    else
        matrixData = thingSpeakRead(readChannelID, ...
            'Fields', [temperatureField, humidityField, rainIntensityField], ...
            'NumPoints', numPoints, ...
            'ReadKey', readAPIKey);
    end

    if size(matrixData, 2) < 3
        error(['Unable to map source fields from ThingSpeak response. ' ...
            'Ensure source channel has numeric data in field1/field2/field3.']);
    end

    temperature = matrixData(:, 1);
    humidity = matrixData(:, 2);
    rainIntensity = matrixData(:, 3);
end

validMask = ~isnan(temperature) & ~isnan(humidity) & ~isnan(rainIntensity);
temperature = temperature(validMask);
humidity = humidity(validMask);
rainIntensity = rainIntensity(validMask);

if isempty(rainIntensity)
    error('No valid ThingSpeak data found. Check channel, fields, and keys.');
end

if numel(rainIntensity) < minValidPoints
    fprintf(['Only %d valid points available (minimum %d). ' ...
        'Skipping forecast/write until more data arrives.\n'], numel(rainIntensity), minValidPoints);
    return;
end

% ----------------------
% Compute lean forecast metrics
% ----------------------
% Basic trend slope using sample index as time proxy.
idx = (1:numel(rainIntensity))';
if numel(rainIntensity) > 1
    pRain = polyfit(idx, rainIntensity, 1);
    rainTrendPerSample = pRain(1);
    pTemp = polyfit(idx, temperature, 1);
    pHum = polyfit(idx, humidity, 1);
else
    rainTrendPerSample = 0;
    pTemp = [0, temperature(end)];
    pHum = [0, humidity(end)];
end

latestTemp = temperature(end);
latestHum = humidity(end);
latestRain = rainIntensity(end);

% Near-term heuristic nowcast score (0..100).
rainChance1h = 100 * ( ...
    0.55 * min(max(latestRain, 0), 1) + ...
    0.30 * min(max(latestHum / 100, 0), 1) + ...
    0.15 * min(max((rainTrendPerSample + 0.2) / 0.4, 0), 1));

% Expected rain intensity in the next horizon.
expectedRainIntensity1h = min(max(latestRain + rainTrendPerSample * 10, 0), 1);

% A simple categorical forecast code:
% 0 = clear/dry, 1 = humid/cloudy, 2 = light rain likely, 3 = heavy rain likely
if rainChance1h >= 75 || expectedRainIntensity1h >= 0.8
    forecastCode = 3;
elseif rainChance1h >= 50 || expectedRainIntensity1h >= 0.5
    forecastCode = 2;
elseif latestHum >= 70
    forecastCode = 1;
else
    forecastCode = 0;
end

% Confidence rises with more samples and smoother rain signal.
sampleConfidence = min(numel(rainIntensity) / 60, 1);
smoothnessPenalty = min(std(rainIntensity) / 0.4, 1);
confidencePercent = 100 * max(0.2, 0.7 * sampleConfidence + 0.3 * (1 - smoothnessPenalty));

% 1-hour rough trend projections for display/logging only.
tempForecast1h = pTemp(1) * (numel(temperature) + 10) + pTemp(2);
humidityForecast1h = pHum(1) * (numel(humidity) + 10) + pHum(2);
humidityForecast1h = min(max(humidityForecast1h, 0), 100);

fprintf('Samples analyzed: %d\n', numel(rainIntensity));
fprintf('Nowcast rain chance 1h: %.1f%%\n', rainChance1h);
fprintf('Expected rain intensity 1h: %.2f\n', expectedRainIntensity1h);
fprintf('Forecast code: %d\n', forecastCode);
fprintf('Confidence: %.1f%%\n', confidencePercent);
fprintf('Temp forecast 1h: %.2f\n', tempForecast1h);
fprintf('Humidity forecast 1h: %.2f\n', humidityForecast1h);

% ----------------------
% Write derived metrics
% ----------------------
% Writes exactly one update per run to avoid request bursts.
% Destination field mapping (recommended):
% field1 = rain_chance_1h_percent
% field2 = expected_rain_intensity_1h
% field3 = forecast_code
% field4 = confidence_percent
if readChannelID == writeChannelID && ~allowSameChannelWrite
    fprintf(['Write skipped: readChannelID and writeChannelID are the same. ' ...
        'Use a separate destination channel or set allowSameChannelWrite=true.\n']);
else
    try
        if isempty(writeAPIKey)
            error('writeAPIKey is empty. Set destination channel Write API key.');
        end

        thingSpeakWrite(writeChannelID, [rainChance1h, expectedRainIntensity1h, forecastCode, confidencePercent], ...
            'Fields', [1, 2, 3, 4], ...
            'WriteKey', writeAPIKey);
    catch ME
        if contains(lower(ME.message), 'too frequent')
            fprintf(['Write skipped due to ThingSpeak rate limit. ' ...
                'Ensure writes are >=15s apart and prefer a dedicated destination channel.\n']);
        elseif contains(lower(ME.message), 'valid read api key')
            error(['Read failed: verify readChannelID/readAPIKey pair belongs to the SAME source channel, ' ...
                'and ensure the key has no extra spaces.']);
        else
            rethrow(ME);
        end
    end
end

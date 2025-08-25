% Parameters
N = 16;           % Length of moving average filter
alpha = 1/16;      % Smoothing factor for exponential moving average
fs = 1;        % Sampling frequency in Hz
Nfft = 1024;      % FFT points

% Frequency vector
f = linspace(0, fs/2, Nfft/2+1);

% Moving Average Filter
% Impulse response: h[n] = 1/N for n = 0 to N-1
ma_filter = ones(1, N)/N;
ma_freq = fft(ma_filter, Nfft);
ma_mag = abs(ma_freq(1:Nfft/2+1));

% Exponential Moving Average Filter
% Impulse response: h[n] = alpha * (1-alpha)^n for n >= 0
n = 0:128-1;
ema_filter = alpha * (1-alpha).^n;
ema_filter
ema_freq = fft(ema_filter, Nfft);
ema_mag = abs(ema_freq(1:Nfft/2+1));

% Plotting
figure;
plot(f, 20*log10(ma_mag), 'b-', 'LineWidth', 2, 'DisplayName', 'Moving Average');
hold on;
plot(f, 20*log10(ema_mag), 'r--', 'LineWidth', 2, 'DisplayName', ['Exponential MA (\alpha=' num2str(alpha) ')']);
grid on;

% Plot settings
title('Magnitude Spectrum of Moving Average and Exponential Moving Average Filters');
xlabel('Frequency (Hz)');
ylabel('Magnitude (dB)');
legend('show');
set(gca, 'FontSize', 12);
pause

%% arduino connection
arduinoPort = "COM4"; 
baudRate = 115200;
s = serialport(arduinoPort, baudRate);
configureTerminator(s, "LF"); %line feed 
flush(s); % clean the port
%%
%filter and sampling rate of the data
fs = 100; %sampling rate
f_high = 4;
f_low = 0.5;
[b , a] = butter(3, [f_low, f_high]/(fs/2), 'bandpass'); %range  filter
[bm , am ]= butter(1,5/(fs/2), "low"); %lowpass motion filter
[bs , as]= butter(1,1/(fs/2), "low"); %lowpass sweat filter
%filters memory
zh_size = max(length(a), length(b)) - 1; % start the memory from (n-1)
zm_size = max(length(am), length(bm)) - 1;
zs_size = max(length(as), length(bs)) - 1;
%%
% heart
subplot(3,1,1);
hploth = plot(nan,nan,"g","LineWidth",3);
xlim([1 500]);
ylim("auto");
title('Heart')
xlabel("time");
ylabel("value");
%%
% motion
subplot(3,1,2);
hplotm = plot(nan,nan,"b","LineWidth",3);
xlim([1 500]);
ylim([-2 4]);
title('Motion')
xlabel("time");
ylabel("value");
%%
%gsr
subplot(3,1,3);
hplots = plot(nan,nan,"y","LineWidth",3);
xlim([1 500]);
ylim([-1 5]);
title('GSR')
xlabel("time");
ylabel("value");
all_plots = [hploth,hplotm,hplots]; % the mat of all plots
%% data structure for the main function
databox = struct(...
    "heart", zeros(1,500), "motion", zeros(1,500), "sweat", zeros(1,500), ...
    'count', 1, 'b', b, 'a', a, 'bm', bm, 'am', am, 'bs', bs, 'as', as, 'fs', fs, ...
    "bpm_history", zeros(1,20), "countpanic", 0, "s", s, ...
    "zh", zeros(zh_size, 1), ...
    "zm", zeros(zm_size, 1), ...
    "zs", zeros(zs_size, 1));          
%% the timer function
t = timer("ExecutionMode","fixedRate","BusyMode","drop","Period",1/fs,"TimerFcn",{@simdata,all_plots});
t.UserData = databox;
start(t);
%% the main function
function simdata(src, ~, all_plots) 
    s_port = src.UserData.s;
    new_data_flag = false;
    
    % Drain the buffer completely to eliminate lag
    while s_port.NumBytesAvailable > 0
        try
            % import and split the data according to ","
            dataStr = readline(s_port);
            dataValues = str2double(split(dataStr, ","));
            % put the data in the correct order heart--->motion--->gsr
            if length(dataValues) == 3 && ~any(isnan(dataValues))
               
                real_heart = dataValues(1);
                real_motion = dataValues(2);
                real_sweat = dataValues(3);
                
                % Filter sample-by-sample a filter function of two outputs
                [filt_h, src.UserData.zh] = filter(src.UserData.b, src.UserData.a, real_heart, src.UserData.zh);
                [filt_m, src.UserData.zm] = filter(src.UserData.bm, src.UserData.am, real_motion, src.UserData.zm);
                [filt_s, src.UserData.zs] = filter(src.UserData.bs, src.UserData.as, real_sweat, src.UserData.zs);
                
                % Buffer the FILTERED data
                src.UserData.heart = [src.UserData.heart(2:end), filt_h];
                src.UserData.motion = [src.UserData.motion(2:end), filt_m];
                src.UserData.sweat = [src.UserData.sweat(2:end), filt_s];
                
                src.UserData.count = src.UserData.count + 1;
                new_data_flag = true;
            end
        catch
            break; 
        end
    end
    
    count = src.UserData.count;
    fs = src.UserData.fs;

    %  Process and Plot ONLY if new data was actually read
    if new_data_flag && mod(count, 20) == 0 % for every 20 laps reducing cpu effort
        % Plot the already-filtered buffers
        set(all_plots(1), 'YData', src.UserData.heart, 'XData', 1:500);
        set(all_plots(2), 'YData', src.UserData.motion, 'XData', 1:500); 
        set(all_plots(3), 'YData', src.UserData.sweat, 'XData', 1:500);
      %%   bpm calculation
[pks, locs] = findpeaks(src.UserData.heart, ...
    "MinPeakProminence", 0.8 * std(src.UserData.heart), ...
    "MinPeakDistance", floor(fs * 0.35)); %about 148 bpm max and 45 min
%bpm fail safe
        if length(locs) >= 2
            
            intervals = diff(locs) / fs; %the time taken between two beats in "sec" (sec per beat)
            valid_intervals = intervals(intervals > 0.3 & intervals < 1.5);
            
            if ~isempty(valid_intervals) % if the interval is NOT empty 
               
                raw_bpm = 60 / median(valid_intervals); % converting into bpm, why median not mean ? -----> the sensor shock 
                raw_bpm = max(45, min(raw_bpm, 150)); % normal range of human being 45 to 150
     
                if src.UserData.bpm_history(end) == 0 % put the new data in the history
                    smoothed_bpm = raw_bpm;
                else
                    bpm_difference = abs(raw_bpm - src.UserData.bpm_history(end)); % the slop of the bpm fail safe from the oscillations 
                    
                    if bpm_difference > 15 % if the difference is higher than 15 then increase the bpm gradually 
                        smoothed_bpm = 0.2 * raw_bpm + 0.8 * src.UserData.bpm_history(end);5  
                    else         %if not then treat it like noise 
                        smoothed_bpm = 0.02 * raw_bpm + 0.98 * src.UserData.bpm_history(end); 
                    end
                end
                          
                proposed_bpm = round(smoothed_bpm); % round the bpm to the nearest decimal 
                last_bpm = round(src.UserData.bpm_history(end)); %round the history to the nearest decimal 
                
                if last_bpm > 0 && abs(proposed_bpm - last_bpm) <= 1 % if the change is lower than 1 it is noise then ignore it
                    bpm = last_bpm;
                    smoothed_bpm = last_bpm; 
                else
                    bpm = proposed_bpm;
                end
                % Update history
                src.UserData.bpm_history = [src.UserData.bpm_history(2:end), smoothed_bpm];
                
                bpm = round(smoothed_bpm); 
            else
                % Fallback if valid_intervals is empty
                bpm = round(mean(nonzeros(src.UserData.bpm_history)));
            end
        else
            title(all_plots(1).Parent, 'Calculating...');
            bpm = round(mean(nonzeros(src.UserData.bpm_history)));
        end
        if isnan(bpm), bpm = 0; end % if the bpm mat is nan
   
        %%
        % applying the thresholds for the logic of state of the user
        if length(locs) >= 2  
            thresh_motion = 0.25; 
            thresh_sweat = 1.2; 
            
            % Calculate against the already-filtered buffers
            recent_motion = std(src.UserData.motion(end-100:end)); % the stranded deviation from the mean of motion
            recent_sweat = mean(src.UserData.sweat(end-100:end)); % the mean of the sweat sensor
            
            valid_history = nonzeros(src.UserData.bpm_history);
            if isempty(valid_history)
                baseline_bpm = bpm;
            else
                baseline_bpm = mean(valid_history);
            end
            
            bpm_slop = bpm - baseline_bpm; %the change between two successive bpm for panic attack detection
            
            % LOGIC of the state of the user 
            if recent_motion >= thresh_motion
                if bpm >= 100
                   current_state = "State: Active / Moving";
                else
                    current_state = "State: Normal Light Activity";
                end
            else
                if bpm >= 100 && bpm_slop >= 15 && recent_sweat >= thresh_sweat
                    src.UserData.countpanic = src.UserData.countpanic + 1;
                    if src.UserData.countpanic >= 10
                        current_state = "CRITICAL: PANIC ATTACK DETECTED";
                    else
                        current_state = "Warning: Analyzing potential panic attack... (" + num2str(src.UserData.countpanic) + "/10)";
                    end
                else
                    src.UserData.countpanic = 0;
                    if bpm < 100
                        if recent_sweat < thresh_sweat
                          current_state = "State: Normal Resting";
                        else
                            current_state = "State: Mildly Anxious / Warm";
                        end
                    elseif bpm >= 100 && bpm < 120
                        if recent_sweat >= thresh_sweat
                            current_state = "State: Stressed";
                        else
                            current_state = "State: Elevated HR";
                        end
                    else
                        current_state = "Warning: Tachycardia Detected";
                    end
                end
            end
            %% the ui looking 
            % detect the quality of the signal
            signal_quality = std(src.UserData.heart(end-100:end));
            if signal_quality < 0.075
                title(all_plots(1).Parent, 'Poor Signal - Adjust Finger', 'Color', 'm');
                drawnow limitrate;
                return;
            end

            
            ui_text = sprintf('Heart Rate: %d BPM   |   %s', bpm, current_state);
            if contains(current_state, "CRITICAL")
                title(all_plots(1).Parent, ui_text, 'Color', 'r', 'FontSize', 14, 'FontWeight', 'bold');
                write(src.UserData.s, '1', "char"); % send the buzzer alarm to arduino
            else
                title(all_plots(1).Parent, ui_text, 'Color', 'k', 'FontSize', 12, 'FontWeight', 'normal');
                write(src.UserData.s, '0', "char");
            end
            
            drawnow limitrate;
        end
    end
    
    % Finish sim
    if count > 10000
        stop(src);
        delete(src);
        disp('Simulation Complete!');
    end
end

         

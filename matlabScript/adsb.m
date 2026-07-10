%% 绘制所有ADSB航迹及样本库航迹
clear all
close all
clc
load('AllTrackCelled.mat');
clear TrackTBD
% load('TrackTBD1.mat');
load('RadialTrackIdx1.mat');
% load('NewResult.mat');
% Result = NewResult;
UnusefulTrackID = [2 5 9 10 11 12 15 20 23 25 26 27 31 32 33 35];
RadialTrackIdx(UnusefulTrackID) = [];
figure
PlotCircle(0, 0, 0, 500, 50, 0, 360, 30, 'km', 400, 225);
% grid on;
hold on
axis equal;
set(gcf, 'Position', [100,100,800,600]);
xlim([-400 120]), ylim([-250 250]);
ax = gca;
% ax.Position = [0.02, 0.02, 0.82, 0.72];

% 绘制所有 ADS-B 航迹（青色）
h1 = [];
for ii = 1:length(TrackADSB)
    a = TrackADSB{ii};
    y = a(:,3).*cosd(a(:,5)).*cosd(a(:,4));
    x = a(:,3).*cosd(a(:,5)).*sind(a(:,4));

    if isempty(h1)
        h1 = plot(x,y,'c','LineWidth',1.5);
    else
        plot(x,y,'c','LineWidth',1.5);
    end
end

% 绘制径向航迹（红色）
h2 = [];
for ii = 1:length(TrackADSB)
    if isempty(find(RadialTrackIdx==ii,1))
        continue;
    end

    a = TrackADSB{ii};
    y = a(:,3).*cosd(a(:,5)).*cosd(a(:,4));
    x = a(:,3).*cosd(a(:,5)).*sind(a(:,4));

    if isempty(h2)
        h2 = plot(x,y,'r','LineWidth',2);
    else
        plot(x,y,'r','LineWidth',2);
    end
end

legend([h1,h2],{'所有目标ADS-B航迹','向站飞行目标ADS-B航迹'},...
       'Location','best');
xlabel('X km','FontSize',13,'FontName','Times New Roman');
ylabel('Y km','FontSize',13,'FontName','Times New Roman');

set(gca,...
    'FontSize',18,...
    'LineWidth',1.2);


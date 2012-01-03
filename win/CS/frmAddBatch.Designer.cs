namespace Handbrake
{
    partial class frmAddBatch
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmAddBatch));
            this.panel1 = new System.Windows.Forms.Panel();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.Label3 = new System.Windows.Forms.Label();
            this.text_destination = new System.Windows.Forms.TextBox();
            this.btn_destBrowse = new System.Windows.Forms.Button();
            this.panel3 = new System.Windows.Forms.Panel();
            this.btn_Cancel = new System.Windows.Forms.Button();
            this.btn_Ok = new System.Windows.Forms.Button();
            this.panel2 = new System.Windows.Forms.Panel();
            this.dgv_DiscTitles = new System.Windows.Forms.DataGridView();
            this.dgvTitles_Include = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            this.dgvTitles_TitleNumber = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dgvTitles_FileName = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dgvTitles_Duration = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.panel1.SuspendLayout();
            this.tableLayoutPanel2.SuspendLayout();
            this.panel3.SuspendLayout();
            this.panel2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgv_DiscTitles)).BeginInit();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.panel2);
            this.panel1.Controls.Add(this.tableLayoutPanel2);
            this.panel1.Controls.Add(this.panel3);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(6, 6);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(638, 561);
            this.panel1.TabIndex = 8;
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.AutoSize = true;
            this.tableLayoutPanel2.ColumnCount = 3;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel2.Controls.Add(this.Label3, 0, 0);
            this.tableLayoutPanel2.Controls.Add(this.text_destination, 1, 0);
            this.tableLayoutPanel2.Controls.Add(this.btn_destBrowse, 2, 0);
            this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.tableLayoutPanel2.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 1;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 29F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(638, 29);
            this.tableLayoutPanel2.TabIndex = 16;
            // 
            // Label3
            // 
            this.Label3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.Label3.AutoSize = true;
            this.Label3.ForeColor = System.Drawing.Color.Black;
            this.Label3.Location = new System.Drawing.Point(3, 8);
            this.Label3.Name = "Label3";
            this.Label3.Size = new System.Drawing.Size(84, 13);
            this.Label3.TabIndex = 0;
            this.Label3.Text = "Output Directory";
            // 
            // text_destination
            // 
            this.text_destination.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.text_destination.Location = new System.Drawing.Point(93, 3);
            this.text_destination.Name = "text_destination";
            this.text_destination.Size = new System.Drawing.Size(461, 20);
            this.text_destination.TabIndex = 1;
            // 
            // btn_destBrowse
            // 
            this.btn_destBrowse.AutoSize = true;
            this.btn_destBrowse.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_destBrowse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_destBrowse.Location = new System.Drawing.Point(560, 3);
            this.btn_destBrowse.Name = "btn_destBrowse";
            this.btn_destBrowse.Size = new System.Drawing.Size(75, 23);
            this.btn_destBrowse.TabIndex = 2;
            this.btn_destBrowse.Text = "Browse";
            this.btn_destBrowse.UseVisualStyleBackColor = true;
            // 
            // panel3
            // 
            this.panel3.Controls.Add(this.btn_Cancel);
            this.panel3.Controls.Add(this.btn_Ok);
            this.panel3.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel3.Location = new System.Drawing.Point(0, 531);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(638, 30);
            this.panel3.TabIndex = 15;
            // 
            // btn_Cancel
            // 
            this.btn_Cancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btn_Cancel.AutoSize = true;
            this.btn_Cancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btn_Cancel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_Cancel.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_Cancel.Location = new System.Drawing.Point(571, 4);
            this.btn_Cancel.Name = "btn_Cancel";
            this.btn_Cancel.Size = new System.Drawing.Size(61, 23);
            this.btn_Cancel.TabIndex = 18;
            this.btn_Cancel.Text = "Cancel";
            this.btn_Cancel.UseVisualStyleBackColor = true;
            this.btn_Cancel.Click += new System.EventHandler(this.btn_Cancel_Click);
            // 
            // btn_Ok
            // 
            this.btn_Ok.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btn_Ok.AutoSize = true;
            this.btn_Ok.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btn_Ok.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_Ok.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_Ok.Location = new System.Drawing.Point(504, 4);
            this.btn_Ok.Name = "btn_Ok";
            this.btn_Ok.Size = new System.Drawing.Size(61, 23);
            this.btn_Ok.TabIndex = 17;
            this.btn_Ok.Text = "OK";
            this.btn_Ok.UseVisualStyleBackColor = true;
            this.btn_Ok.Click += new System.EventHandler(this.btn_Ok_Click);
            // 
            // panel2
            // 
            this.panel2.Controls.Add(this.dgv_DiscTitles);
            this.panel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel2.Location = new System.Drawing.Point(0, 29);
            this.panel2.Name = "panel2";
            this.panel2.Padding = new System.Windows.Forms.Padding(3);
            this.panel2.Size = new System.Drawing.Size(638, 502);
            this.panel2.TabIndex = 17;
            // 
            // dgv_DiscTitles
            // 
            this.dgv_DiscTitles.AllowUserToAddRows = false;
            this.dgv_DiscTitles.AllowUserToDeleteRows = false;
            this.dgv_DiscTitles.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this.dgv_DiscTitles.BackgroundColor = System.Drawing.SystemColors.Window;
            this.dgv_DiscTitles.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dgv_DiscTitles.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.dgvTitles_Include,
            this.dgvTitles_TitleNumber,
            this.dgvTitles_FileName,
            this.dgvTitles_Duration});
            this.dgv_DiscTitles.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dgv_DiscTitles.Location = new System.Drawing.Point(3, 3);
            this.dgv_DiscTitles.Margin = new System.Windows.Forms.Padding(6);
            this.dgv_DiscTitles.Name = "dgv_DiscTitles";
            this.dgv_DiscTitles.RowHeadersBorderStyle = System.Windows.Forms.DataGridViewHeaderBorderStyle.None;
            this.dgv_DiscTitles.RowHeadersVisible = false;
            this.dgv_DiscTitles.Size = new System.Drawing.Size(632, 496);
            this.dgv_DiscTitles.TabIndex = 18;
            // 
            // dgvTitles_Include
            // 
            this.dgvTitles_Include.DataPropertyName = "Include";
            this.dgvTitles_Include.FillWeight = 10F;
            this.dgvTitles_Include.HeaderText = "";
            this.dgvTitles_Include.Name = "dgvTitles_Include";
            // 
            // dgvTitles_TitleNumber
            // 
            this.dgvTitles_TitleNumber.DataPropertyName = "TitleNumber";
            this.dgvTitles_TitleNumber.FillWeight = 25F;
            this.dgvTitles_TitleNumber.HeaderText = "Title Number";
            this.dgvTitles_TitleNumber.Name = "dgvTitles_TitleNumber";
            this.dgvTitles_TitleNumber.ReadOnly = true;
            // 
            // dgvTitles_FileName
            // 
            this.dgvTitles_FileName.DataPropertyName = "FileName";
            this.dgvTitles_FileName.HeaderText = "File Name";
            this.dgvTitles_FileName.Name = "dgvTitles_FileName";
            // 
            // dgvTitles_Duration
            // 
            this.dgvTitles_Duration.DataPropertyName = "Duration";
            this.dgvTitles_Duration.FillWeight = 20F;
            this.dgvTitles_Duration.HeaderText = "Duration";
            this.dgvTitles_Duration.Name = "dgvTitles_Duration";
            this.dgvTitles_Duration.ReadOnly = true;
            // 
            // frmAddBatch
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(650, 573);
            this.Controls.Add(this.panel1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "frmAddBatch";
            this.Padding = new System.Windows.Forms.Padding(6);
            this.Text = "Add Batch...";
            this.Load += new System.EventHandler(this.frmAddBatch_Load);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.tableLayoutPanel2.ResumeLayout(false);
            this.tableLayoutPanel2.PerformLayout();
            this.panel3.ResumeLayout(false);
            this.panel3.PerformLayout();
            this.panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.dgv_DiscTitles)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.Button btn_Cancel;
        private System.Windows.Forms.Button btn_Ok;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
        internal System.Windows.Forms.Label Label3;
        internal System.Windows.Forms.TextBox text_destination;
        private System.Windows.Forms.Button btn_destBrowse;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.DataGridView dgv_DiscTitles;
        private System.Windows.Forms.DataGridViewCheckBoxColumn dgvTitles_Include;
        private System.Windows.Forms.DataGridViewTextBoxColumn dgvTitles_TitleNumber;
        private System.Windows.Forms.DataGridViewTextBoxColumn dgvTitles_FileName;
        private System.Windows.Forms.DataGridViewTextBoxColumn dgvTitles_Duration;


    }
}